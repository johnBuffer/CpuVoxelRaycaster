#pragma once
#include <glm/glm.hpp>


// Lightweight implementation
struct LRay
{
	LRay(const glm::vec3& start_, const glm::vec3& direction_)
		: start(start_)
		, direction(glm::normalize(direction_))
		, inv_direction(1.0f / direction_)
		, offset(-start_ / direction_)
	{}

	const glm::vec3 getT(const glm::vec3& planes) const
	{
		return planes * inv_direction + offset;
	}

	const glm::vec3 start;
	const glm::vec3 direction;
	const glm::vec3 inv_direction;
	const glm::vec3 offset;
};
