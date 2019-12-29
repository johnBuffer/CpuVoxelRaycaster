#include <iostream>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "svo.hpp"
#include "utils.hpp"
#include "swarm.hpp"
#include "FastNoise.h"
#include "raycaster.hpp"


constexpr float PI = 3.141592653;


int32_t main()
{
	constexpr uint32_t win_width = 1600;
	constexpr uint32_t win_height = 900;

	sf::RenderWindow window(sf::VideoMode(win_width, win_height), "Voxel", sf::Style::Default);

	constexpr float render_scale = 1.0f;
	constexpr uint32_t RENDER_WIDTH  = win_width  * render_scale;
	constexpr uint32_t RENDER_HEIGHT = win_height * render_scale;
	sf::RenderTexture render_tex;
	sf::RenderTexture denoised_tex;
	render_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	denoised_tex.create(RENDER_WIDTH, RENDER_HEIGHT);
	render_tex.setSmooth(true);

	float movement_speed = 3.0f;
	float camera_horizontal_angle = 0;
	float camera_vertical_angle = 0;

	glm::vec3 start(10, 10, 10);
	const glm::vec3 camera_origin(float(RENDER_WIDTH) * 0.5f, float(RENDER_HEIGHT) * 0.5f, -0.75f * float(RENDER_WIDTH));
	
	constexpr int32_t grid_size_x = 256;
	constexpr int32_t grid_size_y = 128;
	constexpr int32_t grid_size_z = 256;
	Grid3D<grid_size_x, grid_size_y, grid_size_z>* grid_raw = new Grid3D<grid_size_x, grid_size_y, grid_size_z>();
	Grid3D<grid_size_x, grid_size_y, grid_size_z>& grid = *grid_raw;

	FastNoise myNoise; // Create a FastNoise object
	myNoise.SetNoiseType(FastNoise::SimplexFractal); // Set the desired noise type

	for (int x = 0; x < grid_size_x; x++) {
		for (int z = 0; z < grid_size_z; z++) {
			int max_height = grid_size_y;
			int height = 60.0f * myNoise.GetNoise(x, z);

			grid.setCell(Cell::Mirror, x, grid_size_y - 1, z);

			for (int y(1); y < std::min(max_height, height); ++y) {
				grid.setCell(Cell::Solid, x, grid_size_y - y - 1, z);
			}
		}
	}

	/*for (int i(520); i--;) {
		grid.setCell(false, rand()% grid_size_x, rand() % grid_size_y, rand() % grid_size_z);
	}*/

	sf::VertexArray screen_pixels(sf::Points, RENDER_WIDTH * RENDER_HEIGHT);

	RayCaster raycaser(grid, screen_pixels, sf::Vector2i(RENDER_WIDTH, RENDER_HEIGHT));

	swrm::Swarm swarm(16);

	sf::Mouse::setPosition(sf::Vector2i(win_width / 2, win_height / 2), window);

	float time = 0.0f;

	sf::Shader shader;
	shader.loadFromFile("C:/Users/jeant/Documents/Code/cpp/CpuVoxelRaycaster/res/median_3.frag", sf::Shader::Fragment);

	bool mouse_control = true;
	bool use_denoise = true;

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
		
		const glm::vec3 light_position = glm::vec3(grid_size_x*0.5f, 0.0f, grid_size_z*0.5f) + glm::rotate(glm::vec3(0.0f, 50.0f, grid_size_x*0.5f), 0.2f * time, glm::vec3(0.0f, 1.0f, 0.0f));
		raycaser.setLightPosition(light_position);

		auto group = swarm.execute([&](uint32_t thread_id, uint32_t max_thread) {
			const uint32_t area_width = RENDER_WIDTH / 4;
			const uint32_t area_height = RENDER_HEIGHT / 4;
			const uint32_t start_x = thread_id % 4;
			const uint32_t start_y = thread_id / 4;

			for (int x(start_x * area_width); x < (start_x + 1) * area_width; ++x) {
				for (int y(start_y * area_height); y < (start_y + 1) * area_height; ++y) {
					//const float x = rand() % area_width + start_x * area_width;
					//const float y = rand() % area_height + start_y * area_height;

					const glm::vec3 screen_position(x, y, 0.0f);
					glm::vec3 ray = glm::rotate(screen_position - camera_origin, camera_vertical_angle, glm::vec3(1.0f, 0.0f, 0.0f));
					ray = glm::rotate(ray, camera_horizontal_angle, glm::vec3(0.0f, 1.0f, 0.0f));

					raycaser.cast_ray(sf::Vector2i(x, y), start, glm::normalize(ray), time);
				}
			}
		}, 16);

		group.waitExecutionDone();

		window.clear(sf::Color::Black);

		render_tex.draw(sf::Sprite(denoised_tex.getTexture()));
		render_tex.draw(screen_pixels);
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