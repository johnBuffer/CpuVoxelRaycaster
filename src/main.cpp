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
#include "fly_controller.hpp"
#include "replay.hpp"
#include "event_manager.hpp"
#include "lsvo.hpp"


int32_t main()
{
	const glm::vec3 test_block(63.0f, 63.0f, 63.0f);
	const glm::vec3 camera_origin(1.f, 1.f, 1.f);
	//const glm::vec3 camera_ray(-1.0f, -1.0f, -1.0f);
	const glm::vec3 camera_ray = glm::normalize(test_block - camera_origin);

	SVO svo;
	svo.setCell(Cell::Solid, Cell::Grass, test_block.x, test_block.y, test_block.z);

	constexpr uint32_t size = 3;
	const float side = std::pow(2, size);
	const float scale = 1.0f / side;
	LSVO<6U> lsvo(svo);

	HitPoint result = lsvo.castRay(camera_origin * scale, camera_ray, 1024);

	std::cout << result.complexity << std::endl;

	return 0;
}