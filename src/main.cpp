#include <iostream>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "svo.hpp"
#include "utils.hpp"
#include "swarm.hpp"
#include "FastNoise.h"
#include "raycaster.hpp"
#include "dynamic_blur.hpp"


constexpr float PI = 3.141592653;


int32_t main()
{
	constexpr uint32_t win_width = 1280;
	constexpr uint32_t win_height = 720;

	sf::RenderWindow window(sf::VideoMode(win_width, win_height), "Voxels", sf::Style::Default);
	window.setMouseCursorVisible(false);

	constexpr float render_scale = 0.5f;
	constexpr uint32_t RENDER_WIDTH  = win_width  * render_scale;
	constexpr uint32_t RENDER_HEIGHT = win_height * render_scale;
	sf::RenderTexture render_tex;
	sf::RenderTexture denoised_tex;
	sf::RenderTexture bloom_tex;
	render_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	denoised_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	bloom_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	render_tex.setSmooth(true);

	Blur blur(RENDER_WIDTH, RENDER_HEIGHT, 2.0f);

	float movement_speed = 2.5f;
	float camera_horizontal_angle = 0;
	float camera_vertical_angle = 0;

	const float body_radius = 0.4f;
	glm::vec3 start(10, 120, 10);
	const glm::vec3 camera_origin(float(RENDER_WIDTH) * 0.5f, float(RENDER_HEIGHT) * 0.5f, -0.85f * float(RENDER_WIDTH));
	
	constexpr int32_t grid_size_x = 256;
	constexpr int32_t grid_size_y = 192;
	constexpr int32_t grid_size_z = 256;
	Grid3D<grid_size_x, grid_size_y, grid_size_z>* grid_raw = new Grid3D<grid_size_x, grid_size_y, grid_size_z>();
	Grid3D<grid_size_x, grid_size_y, grid_size_z>& grid = *grid_raw;

	FastNoise myNoise; // Create a FastNoise object
	myNoise.SetNoiseType(FastNoise::SimplexFractal); // Set the desired noise type

	for (int x = 0; x < grid_size_x; x++) {
		for (int z = 0; z < grid_size_z; z++) {
			int max_height = grid_size_y;
			int height = 32.0f * myNoise.GetNoise(x, z);

			grid.setCell(Cell::Water, x, grid_size_y - 1, z);

			for (int y(1); y < std::min(max_height, height); ++y) {
				grid.setCell(Cell::Grass, x, grid_size_y - y - 1, z);
			}
		}
	}

	for (int x = 0; x < grid_size_x; x++) {
		for (int z = 0; z < grid_size_z; z++) {
			int max_height = grid_size_y;

			grid.setCell(Cell::Mirror, x, 0, z);
		}
	}

	for (int x = 100; x < 150; x++) {
		for (int y(10); y < 60; ++y) {
			if (x == 100 || x == 149 || y == 10 || y == 59) {
				grid.setCell(Cell::Solid, x, grid_size_y - y - 1, 50);
			}
			else {
				grid.setCell(Cell::Mirror, x, grid_size_y - y - 1, 50);
			}
		}
	}

	for (int x = 100; x < 150; x++) {
		for (int y(10); y < 60; ++y) {
			if (x == 100 || x == 149 || y == 10 || y == 59) {
				grid.setCell(Cell::Solid, x, grid_size_y - y - 1, 150);
			}
			else {
				grid.setCell(Cell::Mirror, x, grid_size_y - y - 1, 150);
			}
		}
	}

	for (int z = 50; z < 150; z++) {
		for (int y(10); y < 60; ++y) {
			if (z == 50 || z == 149 || y == 10 || y == 59) {
				grid.setCell(Cell::Solid, 24, grid_size_y - y - 1, z);
			}
			else {
				grid.setCell(Cell::Mirror, 24, grid_size_y - y - 1, z);
			}
		}
	}

	/*for (int i(12000); i--;) {
		grid.setCell(Cell::Solid, rand()% grid_size_x, rand() % grid_size_y, rand() % grid_size_z);
	}*/

	sf::VertexArray screen_pixels(sf::Points, RENDER_WIDTH * RENDER_HEIGHT);

	RayCaster raycaser(grid, screen_pixels, sf::Vector2i(RENDER_WIDTH, RENDER_HEIGHT));

	swrm::Swarm swarm(16);

	sf::Mouse::setPosition(sf::Vector2i(win_width / 2, win_height / 2), window);

	float time = 0.0f;

	sf::Shader shader;
	sf::Shader shader_threshold;
	sf::Shader shader_denoise;
	shader.loadFromFile("C:/Users/jeant/Documents/Code/cpp/CpuVoxelRaycaster/res/median_3.frag", sf::Shader::Fragment);
	shader_threshold.loadFromFile("C:/Users/jeant/Documents/Code/cpp/CpuVoxelRaycaster/res/threshold.frag", sf::Shader::Fragment);
	shader_threshold.loadFromFile("C:/Users/jeant/Documents/Code/cpp/CpuVoxelRaycaster/res/denoiser.frag", sf::Shader::Fragment);

	bool mouse_control = true;
	bool use_denoise = true;

	int32_t checker_board_offset = 0;

	while (window.isOpen())
	{
		sf::Clock frame_clock;
		const sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);

		glm::vec3 camera_vec = glm::rotate(glm::vec3(0.0f, 0.0f, 1.0f), camera_vertical_angle, glm::vec3(1.0f, 0.0f, 0.0f));
		camera_vec = glm::rotate(camera_vec, camera_horizontal_angle, glm::vec3(0.0f, 1.0f, 0.0f));

		if (mouse_control) {	
			sf::Mouse::setPosition(sf::Vector2i(win_width / 2, win_height / 2), window);
			const float mouse_sensitivity = 0.005f;
			camera_horizontal_angle -= mouse_sensitivity * (win_width  * 0.5f - mouse_pos.x);
			camera_vertical_angle += mouse_sensitivity * (win_height * 0.5f - mouse_pos.y);

			if (camera_vertical_angle > PI * 0.5f) {
				camera_vertical_angle = PI * 0.5f;
			}
			else if (camera_vertical_angle < -PI * 0.5f) {
				camera_vertical_angle = -PI * 0.5f;
			}
		}

		/*if (grid.getCellAt(start + glm::vec3(0.0f, 1.02f, 0.0f)).type == Cell::Empty) {
			start.y += 0.2f;
		}*/

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
					glm::vec3 move = camera_vec * movement_speed;
					/*move.y = 0.0f;
					if (grid.getCellAt(start + glm::vec3(body_radius, 0.0f, 0.0f)).type != Cell::Empty) {
						move.x = 0.0f;
					}
					if (grid.getCellAt(start + glm::vec3(0.0f, 0.0f, body_radius)).type != Cell::Empty) {
						move.z = 0.0f;
					}*/
					start += move;
					break;
				case sf::Keyboard::S:
					start -= camera_vec * movement_speed;
					break;
				case sf::Keyboard::Q:
					start += glm::vec3(-camera_vec.z, 0.0f, camera_vec.x) * movement_speed;
					break;
				case sf::Keyboard::D:
					start -= glm::vec3(-camera_vec.z, 0.0f, camera_vec.x) * movement_speed;
					break;
				case sf::Keyboard::A:
					use_denoise = !use_denoise;
					break;
				case sf::Keyboard::E:
					mouse_control = !mouse_control;
					window.setMouseCursorVisible(!mouse_control);
					break;
				default:
					break;
				}
			}
		}
		
		const glm::vec3 light_position = glm::vec3(grid_size_x*0.5f, 0.0f, grid_size_z*0.5f) + glm::rotate(glm::vec3(0.0f, -500.0f, 1000.0f), 0.2f * time, glm::vec3(0.0f, 1.0f, 0.0f));
		raycaser.setLightPosition(light_position);

		checker_board_offset = 1 - checker_board_offset;

		auto group = swarm.execute([&](uint32_t thread_id, uint32_t max_thread) {
			const uint32_t area_width = RENDER_WIDTH / 4;
			const uint32_t area_height = RENDER_HEIGHT / 4;
			const uint32_t start_x = thread_id % 4;
			const uint32_t start_y = thread_id / 4;

			for (int x(start_x * area_width); x < (start_x + 1) * area_width; ++x) {
				for (int y(start_y * area_height + (x+checker_board_offset)%2); y < (start_y + 1) * area_height; y += 2) {
					const glm::vec3 screen_position(x, y, 0.0f);
					glm::vec3 ray = glm::rotate(screen_position - camera_origin, camera_vertical_angle, glm::vec3(1.0f, 0.0f, 0.0f));
					ray = glm::rotate(ray, camera_horizontal_angle, glm::vec3(0.0f, 1.0f, 0.0f));

					raycaser.render_ray(sf::Vector2i(x, y), start, glm::normalize(ray), time);
				}
			}
		}, 16);

		group.waitExecutionDone();

		window.clear(sf::Color::Black);
		render_tex.draw(screen_pixels);
		render_tex.display();

		/*denoised_tex.draw(sf::Sprite(render_tex.getTexture()), &shader_denoise);
		denoised_tex.display();

		render_tex.draw(sf::Sprite(denoised_tex.getTexture()));
		render_tex.display();*/

		bloom_tex.draw(sf::Sprite(render_tex.getTexture()), &shader_threshold);
		bloom_tex.display();

		render_tex.draw(blur.apply(bloom_tex.getTexture(), 1), sf::BlendAdd);
		render_tex.display();
		
		sf::Sprite render_sprite(render_tex.getTexture());
		render_sprite.setScale(1.0f / render_scale, 1.0f / render_scale);

		if (use_denoise) {
			window.draw(render_sprite, &shader);
		}
		else {
			window.draw(render_sprite);
		}
		window.display();

		time += frame_clock.getElapsedTime().asSeconds();
	}
}