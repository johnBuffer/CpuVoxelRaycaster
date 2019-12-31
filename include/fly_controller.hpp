#pragma once

#include "camera_controller.hpp"


struct FlyController : public CameraController
{
	void move(const glm::vec3& move_vector, Camera& camera, const Volumetric& volumetric) override
	{
		camera.position += move_vector;
	}
};
