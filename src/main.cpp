#include <iostream>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <sstream>
#include <fstream>

#include "svo.hpp"
#include "grid_3d.hpp"
#include "utils.hpp"
#include "swarm/swarm.hpp"
#include "fastnoise/FastNoise.h"
#include "raycaster.hpp"
#include "fly_controller.hpp"
#include "replay.hpp"
#include "event_manager.hpp"
#include "lsvo.hpp"
#include "lsvo_debug.hpp"


int32_t main()
{
	constexpr uint32_t win_width = 1280;
	constexpr uint32_t win_height = 720;

	sf::RenderWindow window(sf::VideoMode(win_width, win_height), "Voxels", sf::Style::Default);
	window.setMouseCursorVisible(false);
	//window.setFramerateLimit(24);

	constexpr float render_scale = 0.75f;
	constexpr uint32_t RENDER_WIDTH = uint32_t(win_width  * render_scale);
	constexpr uint32_t RENDER_HEIGHT = uint32_t(win_height * render_scale);
	sf::RenderTexture render_tex;
	sf::RenderTexture denoised_tex;
	render_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	denoised_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	
	render_tex.setSmooth(false);

	const float body_radius = 0.4f;

	constexpr uint8_t max_depth = 9;
	constexpr int32_t size = 1 << max_depth;
	constexpr int32_t grid_size_x = size;
	constexpr int32_t grid_size_y = size;
	constexpr int32_t grid_size_z = size;
	using Volume = SVO<max_depth>;
	Volume* volume_raw = new Volume();

	Camera camera;
	camera.position = glm::vec3(256, 200, 256);
	camera.view_angle = glm::vec2(0.0f);
	camera.fov = 1.0f;

	FlyController controller;

	EventManager event_manager(window);

	// Building SVO
	std::cout << "Building SVO..." << std::endl;
	FastNoise myNoise;
	myNoise.SetNoiseType(FastNoise::SimplexFractal);
	for (uint32_t x = 0; x < grid_size_x; x++) {
		for (uint32_t z = 0; z < grid_size_z; z++) {
			int32_t max_height = grid_size_y;
			float amp_x = x - grid_size_x * 0.5f;
			float amp_z = z - grid_size_z * 0.5f;
			float ratio = std::pow(1.0f - sqrt(amp_x * amp_x + amp_z * amp_z) / (10.0f * grid_size_x), 256.0f);
			int32_t height = int32_t(64.0f * myNoise.GetNoise(float(0.75f * x), float(0.75f * z)) + 32);

			const int32_t ground_level = 16;
			for (int y(1); y < std::max(ground_level, std::min(max_height, height)); ++y) {
				volume_raw->setCell(Cell::Solid, Cell::Grass, x, y + 256, z);
			}
		}
	}

	for (int y(0); y < 200; ++y) {
		//volume_raw->setCell(Cell::Solid, Cell::Grass, 256, y, 256);
	}

	constexpr float scale = 1.0f / size;
	LSVO<max_depth> lsvo(*volume_raw);
	std::cout << "Done." << std::endl;

	delete volume_raw;

	RayCaster raycaster(lsvo, sf::Vector2i(RENDER_WIDTH, RENDER_HEIGHT));

	const uint32_t thread_count = 16U;
	const uint32_t area_count = uint32_t(sqrt(thread_count));
	swrm::Swarm swarm(thread_count);

	sf::Mouse::setPosition(sf::Vector2i(win_width / 2, win_height / 2), window);

	float time = 0.0f;

	int32_t checker_board_offset = 0;
	uint32_t frame_count = 0U;
	float frame_time = 0.0f;

	while (window.isOpen()) {
		sf::Clock frame_clock;
		const sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

		if (event_manager.mouse_control) {
			sf::Mouse::setPosition(sf::Vector2i(win_width / 2, win_height / 2), window);
			const float mouse_sensitivity = 0.005f;
			controller.updateCameraView(mouse_sensitivity * glm::vec2(mouse_pos.x - win_width * 0.5f, (win_height  * 0.5f) - mouse_pos.y), camera);
		}

		event_manager.processEvents(controller, camera, raycaster);

		// Computing camera's focal length based on aimed point
		HitPoint closest_point = camera.getClosestPoint(lsvo);
		if (closest_point.cell) {
			camera.focal_length = closest_point.distance * size;
		}
		else {
			camera.focal_length = 100.0f;
		}

		const float light_speed = 0.0f;
		const glm::vec3 light_position = glm::vec3(-200, -1000, -300);
		//const glm::vec3 light_position = glm::vec3(300 + 500 * cos(light_speed*time), 0, 256 + 1000 * sin(light_speed*time));
		raycaster.setLightPosition(light_position * scale + glm::vec3(1.0f));

		sf::Clock render_clock;

		// Computing some constants, could be done outside main loop
		const uint32_t area_width = RENDER_WIDTH / area_count;
		const uint32_t area_height = RENDER_HEIGHT / area_count;
		const float aspect_ratio = float(RENDER_WIDTH) / float(RENDER_HEIGHT);
		const uint32_t rays = raycaster.use_samples ? 32000U : 8000U;

		// Change checker board offset ot render the other pixels
		checker_board_offset = 1 - checker_board_offset;
		// The actual raycasting
		auto group = swarm.execute([&](uint32_t thread_id, uint32_t max_thread) {
			const uint32_t start_x = thread_id % 4;
			const uint32_t start_y = thread_id / 4;
			for (uint32_t x(start_x * area_width); x < (start_x + 1) * area_width; ++x) {
				for (uint32_t y(start_y * area_height + (x + checker_board_offset) % 2); y < (start_y + 1) * area_height; y += 2) {
					// Computing ray coordinates in 'lens' space ie in normalized screen space
					const float lens_x = float(x) / float(RENDER_HEIGHT) - aspect_ratio * 0.5f;
					const float lens_y = float(y) / float(RENDER_HEIGHT) - 0.5f;
					// Get ray to cast with stochastic blur baked into it
					const CameraRay camera_ray = camera.getRay(glm::vec2(lens_x, lens_y));
					raycaster.renderRay(sf::Vector2i(x, y), (camera.position + camera_ray.world_rand_offset)*scale + glm::vec3(1.0f), camera_ray.ray, time);
				}
			}
		});
		// Wait for threads to terminate
		group.waitExecutionDone();

		if (raycaster.use_samples) {
			raycaster.samples_to_image();
		}

		// Add some persistence to reduce the noise
		const float old_value_conservation = raycaster.use_samples ? 0.0f : 0.1f;
		sf::RectangleShape cache1(sf::Vector2f(RENDER_WIDTH, RENDER_HEIGHT));
		cache1.setFillColor(sf::Color(255 * old_value_conservation, 255 * old_value_conservation, 255 * old_value_conservation));
		sf::RectangleShape cache2(sf::Vector2f(win_width, win_height));
		const float c2 = 255 * (1.0f - old_value_conservation);
		cache2.setFillColor(sf::Color(c2, c2, c2));
		// Draw image to final render texture
		sf::Texture texture;
		texture.loadFromImage(raycaster.render_image);
		render_tex.draw(sf::Sprite(texture));
		render_tex.draw(cache2, sf::BlendMultiply);
		render_tex.display();
		// Not really denoised but OK
		denoised_tex.draw(cache1, sf::BlendMultiply);
		sf::Sprite render_sprite(render_tex.getTexture());
		denoised_tex.draw(render_sprite, sf::BlendAdd);
		denoised_tex.display();
		// Scale and render
		sf::Sprite final_sprite(denoised_tex.getTexture());
		final_sprite.setScale(1.0f / render_scale, 1.0f / render_scale);
		window.draw(final_sprite);
		window.display();

		const float dt = frame_clock.getElapsedTime().asSeconds();
		++frame_count;
		time += dt;

		//std::cout << "AVG frame time " << time / double(frame_count) << std::endl;
	}
}