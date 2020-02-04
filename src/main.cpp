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
	const glm::vec3 test_block(1.0f, 1.0f, 1.0f);
	const glm::vec3 camera_origin(1.5f, 1.5f, 10.25f);
	const glm::vec3 camera_ray = glm::normalize(test_block - camera_origin);

	SVO svo;
	svo.setCell(Cell::Solid, Cell::Grass, test_block.x, test_block.y, test_block.z);

	LSVO<4U> lsvo(svo);

	lsvo.castRay(camera_origin, camera_ray, 1024);
}