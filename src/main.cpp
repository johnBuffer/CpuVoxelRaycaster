#include <iostream>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "svo.hpp"
#include "utils.hpp"
#include "swarm.hpp"
#include "FastNoise.h"


constexpr float PI = 3.141592653;


int32_t main()
{
	constexpr uint32_t win_width = 800;
	constexpr uint32_t win_height = 800;
	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;

	sf::RenderWindow window(sf::VideoMode(win_width, win_height), "Voxel", sf::Style::Default, settings);

	constexpr float render_ratio = 1.0f;
	constexpr uint32_t RENDER_WIDTH  = win_width  * render_ratio;
	constexpr uint32_t RENDER_HEIGHT = win_height * render_ratio;
	sf::RenderTexture render_tex;
	render_tex.create(RENDER_WIDTH, RENDER_HEIGHT);

	float movement_speed = 3.0f;
	float camera_horizontal_angle = 0;
	float camera_vertical_angle = 0;

	glm::vec3 start(10, 10, 10);
	const glm::vec3 camera_origin(float(RENDER_WIDTH) * 0.5f, float(RENDER_HEIGHT) * 0.5f, -0.75f * float(RENDER_WIDTH));
	
	constexpr int32_t grid_size_x = 128;
	constexpr int32_t grid_size_y = 128;
	constexpr int32_t grid_size_z = 128;
	Grid3D<grid_size_x, grid_size_y, grid_size_z>* grid_raw = new Grid3D<grid_size_x, grid_size_y, grid_size_z>();
	Grid3D<grid_size_x, grid_size_y, grid_size_z>& grid = *grid_raw;

	FastNoise myNoise; // Create a FastNoise object
	myNoise.SetNoiseType(FastNoise::SimplexFractal); // Set the desired noise type

	for (int x = 0; x < grid_size_x; x++) {
		for (int z = 0; z < grid_size_z; z++) {
			int max_height = grid_size_y;
			int height = 30.0f * myNoise.GetNoise(x, z) + 10;

			for (int y(0); y < std::min(max_height, height); ++y) {
				grid.setCell(false, x, grid_size_y - y - 1, z);
			}
		}
	}

	/*for (int i(520); i--;) {
		grid.setCell(false, rand()% grid_size_x, rand() % grid_size_y, rand() % grid_size_z);
	}*/

	sf::VertexArray screen_pixels(sf::Points, win_height * win_width);

	swrm::Swarm swarm(16);

	sf::Mouse::setPosition(sf::Vector2i(win_width / 2, win_height / 2), window);

	float time = 0.0f;

	while (window.isOpen())
	{
		sf::Clock frame_clock;
		const sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

		const float mouse_sensitivity = 0.01f;
		camera_horizontal_angle -= mouse_sensitivity * (win_width  * 0.5f - mouse_pos.x);
		camera_vertical_angle   += mouse_sensitivity * (win_height * 0.5f - mouse_pos.y);

		if (camera_vertical_angle > PI * 0.5f) {
			camera_vertical_angle = PI * 0.5f;
		}
		else if (camera_vertical_angle < -PI * 0.5f) {
			camera_vertical_angle = -PI * 0.5f;
		}

		glm::vec3 camera_vec = glm::rotate(glm::vec3(0.0f, 0.0f, 1.0f), camera_vertical_angle, glm::vec3(1.0f, 0.0f, 0.0f));
		camera_vec = glm::rotate(camera_vec, camera_horizontal_angle, glm::vec3(0.0f, 1.0f, 0.0f));

		sf::Mouse::setPosition(sf::Vector2i(win_width / 2, win_height / 2), window);

		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::KeyPressed)
			{
				switch (event.key.code)
				{
				case sf::Keyboard::Escape:
					window.close();
					break;
				case sf::Keyboard::Z:
					start.x += camera_vec.x*movement_speed;
					start.y += camera_vec.y*movement_speed;
					start.z += camera_vec.z*movement_speed;
					break;
				case sf::Keyboard::S:
					start.x -= camera_vec.x*movement_speed;
					start.y -= camera_vec.y*movement_speed;
					start.z -= camera_vec.z*movement_speed;
					break;
				case sf::Keyboard::Q:
					start.x -= movement_speed;
					break;
				case sf::Keyboard::D:
					start.x += movement_speed;
					break;
				case sf::Keyboard::A:
					start.z += movement_speed;
					break;
				case sf::Keyboard::E:
					start.z -= movement_speed;
					break;
				default:
					break;
				}
			}
		}
		
		const glm::vec3 light_position = glm::rotate(glm::vec3(0.0f, -1000.0f, 1000.0f), 0.2f * time, glm::vec3(0.0f, 1.0f, 0.0f));

		auto group = swarm.execute([&](uint32_t thread_id, uint32_t max_thread) {
			const uint32_t area_width = win_width / 4;
			const uint32_t area_height = win_height / 4;
			const uint32_t start_x = thread_id % 4;
			const uint32_t start_y = thread_id / 4;

			for (int x(start_x * area_width); x < (start_x + 1) * area_width; ++x) {
				for (int y(start_y * area_height); y < (start_y + 1) * area_height; ++y) {

					const glm::vec3 screen_position(x, y, 0.0f);
					glm::vec3 ray = glm::rotate(screen_position - camera_origin, camera_vertical_angle, glm::vec3(1.0f, 0.0f, 0.0f));
					ray = glm::rotate(ray, camera_horizontal_angle, glm::vec3(0.0f, 1.0f, 0.0f));

					const HitPoint intersection = grid.cast_ray(start, ray);

					screen_pixels[x * win_height + y].position = sf::Vector2f(x, y);
					if (intersection.hit) {
						sf::Color color;
						if (intersection.normal.x) {
							color = sf::Color::Red;
						}
						else if (intersection.normal.y) {
							color = sf::Color::Green;
						}
						else if (intersection.normal.z) {
							color = sf::Color::Blue;
						}

						constexpr float eps = 0.001f;
						
						
						const glm::vec3 hit_light = glm::normalize(light_position - intersection.position);
						const HitPoint light_ray = grid.cast_ray(intersection.position + eps * intersection.normal, hit_light);

						float light_intensity = 0.2f;

						if (!light_ray.hit) {
							light_intensity = std::max(0.2f, glm::dot(intersection.normal, hit_light));
						}

						color.r *= light_intensity;
						color.g *= light_intensity;
						color.b *= light_intensity;
						
						screen_pixels[x * win_height + y].color = color;
					}
					else {
						screen_pixels[x * win_height + y].color = sf::Color::Black;
					}
				}
			}
		}, 16);

		group.waitExecutionDone();

		window.clear(sf::Color::Black);

		window.draw(screen_pixels);

		window.display();

		time += frame_clock.getElapsedTime().asSeconds();
	}
}