#pragma once

#include <glm/glm.hpp>
#include "cell.hpp"


struct HitPoint
{
	HitPoint()
		: type(Cell::Empty)
	{}

	HitPoint(bool hit_, float light_intensity_)
		: type(Cell::Empty)
	{}

	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 voxel_coord;

	Cell::Type type;
	float distance;
};


class Volumetric
{
public:
	virtual HitPoint cast_ray(const glm::vec3& position, const glm::vec3& direction) const = 0;
};
