#include <iostream>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <sstream>
#include <fstream>

#include "svo.hpp"
#include "grid_3d.hpp"
#include "utils.hpp"
#include "swarm.hpp"
#include "FastNoise.h"
#include "raycaster.hpp"
#include "dynamic_blur.hpp"
#include "fly_controller.hpp"
#include "replay.hpp"


int32_t main()
{
	constexpr uint32_t win_width = 1280;
	constexpr uint32_t win_height = 720;

	sf::RenderWindow window(sf::VideoMode(win_width, win_height), "Voxels", sf::Style::Default);
	window.setMouseCursorVisible(false);
	//window.setFramerateLimit(30);

	constexpr float render_scale = 1.0f;
	constexpr uint32_t RENDER_WIDTH  = uint32_t(win_width  * render_scale);
	constexpr uint32_t RENDER_HEIGHT = uint32_t(win_height * render_scale);
	sf::RenderTexture render_tex;
	sf::RenderTexture denoised_tex;
	sf::RenderTexture bloom_tex;
	render_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	denoised_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	bloom_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	render_tex.setSmooth(false);

	Blur blur(RENDER_WIDTH, RENDER_HEIGHT, 0.5f);

	float movement_speed = 3.5f;

	const float body_radius = 0.4f;
	
	const glm::vec3 camera_origin(float(RENDER_WIDTH) * 0.5f, float(RENDER_HEIGHT) * 0.5f, -0.85f * float(RENDER_WIDTH));
	
	constexpr int32_t size = 512;
	constexpr int32_t grid_size_x = size;
	constexpr int32_t grid_size_y = size;
	constexpr int32_t grid_size_z = size;
	using Volume = SVO;
	//using Volume = Grid3D<grid_size_x, grid_size_y, grid_size_z>;
	Volume* grid_raw = new Volume();
	Volume& grid = *grid_raw;

	Camera camera;
	camera.position = glm::vec3(-40.6978, 326.232, 98.0843);
	camera.view_angle = glm::vec2(0.0f);

	FlyController controller;

	FastNoise myNoise; // Create a FastNoise object
	myNoise.SetNoiseType(FastNoise::SimplexFractal); // Set the desired noise type

	for (uint32_t x = 0; x < grid_size_x; x++) {
		for (uint32_t z = 0; z < grid_size_z; z++) {
			int32_t max_height = grid_size_y;
			int32_t height = int32_t(64.0f * myNoise.GetNoise(float(0.75f * x), float(0.75f * z)) + 32);

			grid.setCell(Cell::Mirror, Cell::None, x, grid_size_y - 1, z);

			for (int y(1); y < std::min(max_height, height); ++y) {
				grid.setCell(Cell::Solid, Cell::Grass, x, grid_size_y - y - 1, z);
			}
		}
	}

	sf::VertexArray screen_pixels(sf::Points, RENDER_WIDTH * RENDER_HEIGHT);

	RayCaster raycaster(grid, screen_pixels, sf::Vector2i(RENDER_WIDTH, RENDER_HEIGHT));

	const uint32_t thread_count = 16U;
	const uint32_t area_count = uint32_t(sqrt(thread_count));
	swrm::Swarm swarm(thread_count);

	sf::Mouse::setPosition(sf::Vector2i(win_width / 2, win_height / 2), window);

	float time = 0.0f;

	bool mouse_control = true;
	bool use_denoise = false;
	bool mode_demo = false;

	int32_t checker_board_offset = 0;

	sf::Texture screen_capture;
	screen_capture.create(win_width, win_height);
	uint32_t frame_count = 0U;

	bool left(false), right(false), forward(false), backward(false), up(false);

	float mray_s_mean = 0.0f;
	float value_count = 0.0f;

	while (window.isOpen())
	{
		sf::Clock frame_clock;
		const sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

		if (mouse_control) {	
			sf::Mouse::setPosition(sf::Vector2i(win_width / 2, win_height / 2), window);
			const float mouse_sensitivity = 0.005f;
			controller.updateCameraView(mouse_sensitivity * glm::vec2(mouse_pos.x - win_width  * 0.5f, (win_height  * 0.5f) - mouse_pos.y), camera);
		}

		//camera.view_angle.x = 0.975;
		//camera.view_angle.y = -0.41;

		glm::vec3 camera_vec = glm::rotate(glm::vec3(0.0f, 0.0f, 1.0f), camera.view_angle.y, glm::vec3(1.0f, 0.0f, 0.0f));
		camera_vec = glm::rotate(camera_vec, camera.view_angle.x, glm::vec3(0.0f, 1.0f, 0.0f));

		glm::vec3 move = glm::vec3(0.0f);
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::KeyPressed) {
				switch (event.key.code) {
				case sf::Keyboard::Escape:
					window.close();
					break;
				case sf::Keyboard::Z:
					forward = true;
					break;
				case sf::Keyboard::S:
					backward = true;
					break;
				case sf::Keyboard::Q:
					left = true;
					break;
				case sf::Keyboard::D:
					right = true;
					break;
				case sf::Keyboard::O:
					raycaster.use_ao = !raycaster.use_ao;
					break;
				case sf::Keyboard::Space:
					up = true;
					break;
				case sf::Keyboard::A:
					use_denoise = !use_denoise;
					break;
				case sf::Keyboard::E:
					mouse_control = !mouse_control;
					window.setMouseCursorVisible(!mouse_control);
					break;
				case sf::Keyboard::F:
					mode_demo = !mode_demo;
					window.setMouseCursorVisible(!mode_demo && !mouse_control);
					if (mode_demo) {
						time = 0.0f;
					}
					break;
				default:
					break;
				}
			}
			else if (event.type == sf::Event::KeyReleased) {
				switch (event.key.code) {
				case sf::Keyboard::Z:
					forward = false;
					break;
				case sf::Keyboard::S:
					backward = false;
					break;
				case sf::Keyboard::Q:
					left = false;
					break;
				case sf::Keyboard::D:
					right = false;
					break;
				case sf::Keyboard::Space:
					up = false;
					break;
				default:
					break;
				}
			}
		}

		if (forward) {
			move += camera_vec * movement_speed;
		}
		else if (backward) {
			move -= camera_vec * movement_speed;
		}

		if (left) {
			move += glm::vec3(-camera_vec.z, 0.0f, camera_vec.x) * movement_speed;
		}
		else if (right) {
			move -= glm::vec3(-camera_vec.z, 0.0f, camera_vec.x) * movement_speed;
		}

		if (up) {
			move += glm::vec3(0.0f, -1.0f, 0.0f) * movement_speed;
		}

		controller.move(move, camera);
		
		//const glm::vec3 light_position = glm::vec3(grid_size_x*0.5f, 0.0f, grid_size_z*0.5f) + glm::rotate(glm::vec3(0.0f, 0.0f, 1000.0f), 0.2f * time, glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::vec3 light_position = glm::vec3(0, 250, 700);
		raycaster.setLightPosition(light_position);

		checker_board_offset = 1 - checker_board_offset;

		sf::Clock render_clock;
		uint32_t ray_count = 0U;
		auto group = swarm.execute([&](uint32_t thread_id, uint32_t max_thread) {
			const uint32_t area_width = RENDER_WIDTH / area_count;
			const uint32_t area_height = RENDER_HEIGHT / area_count;
			const uint32_t start_x = thread_id % 4;
			const uint32_t start_y = thread_id / 4;
			for (uint32_t x(start_x * area_width); x < (start_x + 1) * area_width; ++x) {
				for (uint32_t y(start_y * area_height + (x + checker_board_offset) % 2); y < (start_y + 1) * area_height; y += 2) {
					
						/*for (uint32_t i(4000); i--;) {
							const uint32_t x = start_x * area_width + rand() % area_width;
							const uint32_t y = start_y * area_height + rand() % area_height;*/
					++ray_count;
					const glm::vec3 screen_position(x, y, 0.0f);
					glm::vec3 ray = glm::rotate(screen_position - camera_origin, camera.view_angle.y, glm::vec3(1.0f, 0.0f, 0.0f));
					ray = glm::rotate(ray, camera.view_angle.x, glm::vec3(0.0f, 1.0f, 0.0f));

					raycaster.renderRay(sf::Vector2i(x, y), camera.position, glm::normalize(ray), time);
				}
			}
		});

		group.waitExecutionDone();

		const float render_time = render_clock.getElapsedTime().asSeconds();
		value_count += 1.0f;
		float instant_measure = (ray_count / render_time) / 1000000.0f;
		mray_s_mean += instant_measure;
		std::cout << "MRays/s " << instant_measure << " MEAN : " << mray_s_mean / value_count << std::endl;

		std::cout << camera.view_angle.x << " " << camera.view_angle.y << " " << camera.position.x << " " << camera.position.y << " " << camera.position.z << std::endl;

		//window.clear(sf::Color::Black);
		sf::Texture texture;
		texture.loadFromImage(raycaster.render_image);
		render_tex.draw(sf::Sprite(texture));
		render_tex.display();
		
		sf::Sprite render_sprite(render_tex.getTexture());
		render_sprite.setScale(1.0f / render_scale, 1.0f / render_scale);
		window.draw(render_sprite);
		window.display();

		time += frame_clock.getElapsedTime().asSeconds();
	}
}