#pragma once

#include <glm/glm.hpp>
#include "volumetric.hpp"
#include "utils.hpp"


constexpr float PI = 3.141592653f;


struct Camera
{
	glm::vec3 position;
	glm::vec2 view_angle;
};


struct CameraController
{
	virtual void updateCameraView(const glm::vec2& view_angle, Camera& camera)
	{
		camera.view_angle += view_angle;
		clamp(camera.view_angle.y, -PI * 0.5f, PI * 0.5f);
	}

	virtual void move(const glm::vec3& move_vector, Camera& camera) = 0;
};
