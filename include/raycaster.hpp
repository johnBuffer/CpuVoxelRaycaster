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
	const float eps = 0.001f;

	RayCaster(const Volumetric& volumetric_, sf::VertexArray& va_, const sf::Vector2i& render_size_)
		: volumetric(volumetric_)
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
		const HitPoint intersection = volumetric.castRay(start, direction);
		context.complexity += intersection.complexity;
		context.distance += intersection.distance;

		if (intersection.data) {
			const Cell& data = *(intersection.data);

			if (data.type == Cell::Solid) {
				result = getTextureColorFromHitPoint(intersection);
			}
		}

		const uint32_t c = std::min(255U, std::max(0U, context.complexity / 10U));
		add(result, c);
		return result;
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
		if (point.data->texture == Cell::Grass) {
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

	const Volumetric& volumetric;
	
	const sf::Vector2i render_size;
	sf::VertexArray& va;
	
	glm::vec3 light_position;

	sf::Color sky_color = sf::Color(119, 199, 242);

	const uint32_t max_bounds = 4;
	//const sf::Color sky_color = sf::Color(166, 215, 255);
};
