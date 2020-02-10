#pragma once

#include <glm/glm.hpp>
#include "cell.hpp"


struct HitPoint
{
	HitPoint()
		: cell(nullptr)
		, complexity(0u)
	{}

	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 voxel_coord;

	const Cell* cell;
	float distance;

	uint32_t complexity;
};


struct Ray
{
	Ray() = default;

	Ray(const glm::vec3& start_, const glm::vec3& direction_)
		: start(start_)
		, direction(direction_)
		, inv_direction(1.0f / direction_)
		, t(glm::abs(1.0f / direction_))
		, step(direction_.x >= 0.0f ? 1.0f : -1.0f, copysign(1.0f, direction_.y), copysign(1.0f, direction_.z))
		, dir(direction_.x > 0.0f, direction_.y > 0.0f, direction_.z > 0.0f)
		, point()
		, t_total(0.0f)
		, hit_side(0U)
	{
	}

	const glm::vec3 start;
	const glm::vec3 direction;
	const glm::vec3 inv_direction;
	const glm::vec3 t;
	const glm::vec3 step;
	const glm::vec3 dir;
	uint8_t hit_side;
	float t_total;

	HitPoint point;
};


class Volumetric
{
public:
	virtual HitPoint castRay(const glm::vec3& position, glm::vec3 direction, const float, const float) const = 0;
	virtual void setCell(Cell::Type type, Cell::Texture texture, uint32_t x, uint32_t y, uint32_t z) = 0;

};
