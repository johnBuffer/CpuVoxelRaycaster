#pragma once


#include <SFML/Graphics.hpp>
#include "volumetric.hpp"
#include "utils.hpp"


struct RayContext
{
	float distance = 0.0f;
	uint32_t complexity = 0U;
	uint32_t bounds = 0U;
};



struct RayCaster
{
	const float eps = 0.01f;

	RayCaster(const SVO& volumetric_, sf::VertexArray& va_, const sf::Vector2i& render_size_)
		: svo(volumetric_)
		, va(va_)
		, render_size(render_size_)
	{
		image_side.loadFromFile("C:/Users/jeant/Documents/Code/cpp/CpuVoxelRaycaster/res/grass_side_16x16.bmp");
		image_top.loadFromFile("C:/Users/jeant/Documents/Code/cpp/CpuVoxelRaycaster/res/grass_top_16x16.bmp");
	}

	void setLightPosition(const glm::vec3& position)
	{
		light_position = position;
	}

	void renderRay(const sf::Vector2i pixel, const glm::vec3& start, const glm::vec3& direction, float time)
	{
		const uint32_t x = pixel.x;
		const uint32_t y = pixel.y;
		va[x * render_size.y + y].position = sf::Vector2f(float(x), float(y));
		va[x * render_size.y + y].color = sky_color;

		RayContext context;
		context.distance = 0.0f;
		const sf::Color color = castRay(start, direction, 1.5f * time, context);

		va[x * render_size.y + y].color = color;
	}
	

	sf::Color castRay(const glm::vec3& start, const glm::vec3& direction, float time, RayContext& context)
	{
		sf::Color result = sf::Color::Black;
		if (context.bounds > max_bounds)
			return sky_color;

		// Cast ray and check intersection
		const HitPoint intersection = svo.castRay(start, direction, 2048U);
		context.complexity += intersection.complexity;
		context.distance += intersection.distance;

		if (intersection.cell) {
			const Cell& cell = *(intersection.cell);

			if (cell.type == Cell::Solid) {
				result = getTextureColorFromHitPoint(intersection);
				if (use_ao) {
					mult(result, getAmbientOcclusion(intersection));
				}
			}
		}

		return result;
	}

	float getAmbientOcclusion(const HitPoint& point)
	{
		const uint32_t ray_count = 1;
		const uint32_t max_iter = 16;
		const glm::vec3 ao_start = point.position + point.normal * eps;
		float acc = 0.0f;
		for (uint32_t i(ray_count); i--;) {
			glm::vec3 noise_normal = glm::vec3();

			if (point.normal.x) {
				noise_normal = glm::vec3(0.0f, getRand(-100.0f, 100.0f), getRand(-100.0f, 100.0f));
			} else if (point.normal.y) {
				noise_normal = glm::vec3(getRand(-100.0f, 100.0f), 0.0f, getRand(-100.0f, 100.0f));
			} else if (point.normal.z) {
				noise_normal = glm::vec3(getRand(-100.0f, 100.0f), getRand(-100.0f, 100.0f), 0.0f);
			}

			HitPoint ao_point = svo.castRay(ao_start, glm::normalize(point.normal + noise_normal), max_iter);
			if (!ao_point.cell) {
				acc += 1.0f;
			}
		}

		return std::min(1.0f, acc / float(ray_count));
	}

	float getFogValue(const RayContext& context, const HitPoint& point)
	{
		return context.distance * 0.2f;
	}

	const sf::Image& getTextureFromNormal(const glm::vec3& normal)
	{
		if (normal.y) {
			return image_top;
		}

		return image_side;
	}

	const sf::Color getTextureColorFromHitPoint(const HitPoint& point)
	{
		if (point.cell->texture == Cell::Grass) {
			return getColorFromVoxelCoord(getTextureFromNormal(point.normal), point.voxel_coord);
		}
		else {
			return sf::Color::Red;
		}
	}

	const sf::Color getColorFromVoxelCoord(const sf::Image& image, glm::vec2 coords)
	{
		const sf::Vector2u tex_size = image.getSize();
		clamp(coords.x, 0.0f, 1.0f);
		clamp(coords.y, 0.0f, 1.0f);
		return image.getPixel(uint32_t(tex_size.x * coords.x), uint32_t(tex_size.y * coords.y));
	}

	sf::Image image_side;
	sf::Image image_top;

	const SVO& svo;
	
	const sf::Vector2i render_size;
	sf::VertexArray& va;
	
	glm::vec3 light_position;

	sf::Color sky_color = sf::Color(119, 199, 242);

	const uint32_t max_bounds = 4;
	//const sf::Color sky_color = sf::Color(166, 215, 255);

	bool use_ao = false;
};
