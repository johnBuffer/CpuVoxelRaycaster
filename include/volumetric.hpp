#pragma once

#include <glm/glm.hpp>
#include "cell.hpp"


struct HitPoint
{
	HitPoint()
		: data(nullptr)
		, complexity(0)
	{}

	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 voxel_coord;

	const Cell* data;
	float distance;

	uint32_t complexity;
};


class Volumetric
{
public:
	virtual HitPoint castRay(const glm::vec3& position, const glm::vec3& direction) const = 0;
	virtual void setCell(Cell::Type type, Cell::Texture texture, uint32_t x, uint32_t y, uint32_t z) = 0;
};
