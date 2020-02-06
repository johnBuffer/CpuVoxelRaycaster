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
#include "lsvo_debug.hpp"


int32_t main()
{
	constexpr uint32_t octree_depth = 9u;
	const uint32_t side_size = std::pow(2u, octree_depth);
	const float scale = 1.0f / float(side_size);

	const glm::vec3 test_block(511.0f, 511.0f, 511.0f);
	const glm::vec3 camera_origin(1.5f, 1.0f, 1.0f);
	const glm::vec3 camera_ray = glm::normalize(test_block - camera_origin);
	std::cout << "Ray " << toString(camera_ray) << std::endl;

	SVO svo;
	//svo.setCell(Cell::Solid, Cell::Grass, test_block.x, test_block.y, test_block.z);

	LSVO<octree_depth> lsvo(svo);
	print(lsvo);
	std::cout << std::endl;

	HitPoint result = lsvo.castRay(camera_origin * scale, camera_ray, 1024);

	std::cout << result.complexity << std::endl;

	return 0;
}