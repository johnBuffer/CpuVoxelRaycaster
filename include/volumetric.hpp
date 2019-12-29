#pragma once

#include <glm/glm.hpp>


struct HitPoint
{
	HitPoint()
		: hit(false)
		, light_intensity(0.0f)
	{}

	HitPoint(bool hit_, float light_intensity_)
		: hit(hit_)
		, light_intensity(light_intensity_)
	{}

	bool hit;
	float light_intensity;
	glm::vec3 position;
	glm::vec3 normal;
};


class Volumetric
{
public:
	virtual HitPoint cast_ray(const glm::vec3& position, const glm::vec3& direction) const = 0;
};
