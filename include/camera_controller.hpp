#pragma once

#include <glm/glm.hpp>
#include "volumetric.hpp"
#include "utils.hpp"


constexpr float PI = 3.141592653f;

struct CameraRay
{
	glm::vec3 ray;
	glm::vec3 world_rand_offset;
};

struct Camera
{
	glm::vec3 position;
	glm::vec2 view_angle;
	glm::vec3 camera_vec;
	glm::mat3 rot_mat;

	float aperture = 0.0f;
	float focal_length = 1.0f;
	float fov = 1.0f;

	void setViewAngle(const glm::vec2& angle)
	{
		view_angle = angle;
		rot_mat = generateRotationMatrix(view_angle);
		camera_vec = viewToWorld(glm::vec3(0.0f, 0.0f, 1.0f));
	}

	CameraRay getRay(const glm::vec2& lens_position)
	{
		const glm::vec3 screen_position = glm::vec3(lens_position, fov);
		const glm::vec3 ray_initial = screen_position;
		const glm::vec3 focal_point = glm::normalize(ray_initial) * focal_length;
		const glm::vec3 rand_vec = aperture * glm::vec3(getRand(), getRand(), 0.0f);
		const glm::vec3 new_camera_origin = rand_vec;
		const glm::vec3 ray = glm::normalize(focal_point - new_camera_origin);

		CameraRay result;
		result.ray = viewToWorld(ray);
		result.world_rand_offset = viewToWorld(rand_vec);

		return result;
	}

	glm::vec3 viewToWorld(const glm::vec3& v) const
	{
		return v * rot_mat;
	}

	HitPoint getClosestPoint(const Volumetric& volume) const
	{
		return volume.castRay(position, camera_vec, 512U);
	}
};


struct CameraController
{
	virtual void updateCameraView(const glm::vec2& d_view_angle, Camera& camera)
	{
		glm::vec2 new_angle = camera.view_angle + d_view_angle;
		clamp(new_angle.y, -PI * 0.5f, PI * 0.5f);

		camera.setViewAngle(new_angle);
	}

	virtual void move(const glm::vec3& move_vector, Camera& camera) = 0;

	float movement_speed = 0.5f;
};
