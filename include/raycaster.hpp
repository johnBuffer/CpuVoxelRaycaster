#pragma once


#include <SFML/Graphics.hpp>
#include "SVO.hpp"
#include "utils.hpp"


struct RayContext
{
	float distance = 0.0f;
	uint32_t complexity = 0U;
	uint32_t bounds = 0U;
};


struct Sample
{
	double update_count = 0.0f;
	double r = 0.0f;
	double g = 0.0f;
	double b = 0.0f;
};


struct RayCaster
{
	const float eps = 0.001f;

	RayCaster(const SVO& svo_, sf::VertexArray& va_, const sf::Vector2i& render_size_)
		: svo(svo_)
		, va(va_)
		, render_size(render_size_)
	{
		render_image.create(render_size.x, render_size.y);
		image_side.loadFromFile("C:/Users/jeant/Documents/Code/cpp/CpuVoxelRaycaster/res/grass_side_16x16.bmp");
		image_top.loadFromFile("C:/Users/jeant/Documents/Code/cpp/CpuVoxelRaycaster/res/grass_top_16x16.bmp");

		colors.resize(render_size_.x);
		for (auto& v : colors) {
			v.resize(render_size_.y);
		}
	}

	void setLightPosition(const glm::vec3& position)
	{
		light_position = position;
	}

	void renderRay(const sf::Vector2i pixel, const glm::vec3& start, const glm::vec3& direction, float time)
	{
		const uint32_t x = pixel.x;
		const uint32_t y = pixel.y;

		RayContext context;
		context.distance = 0.0f;

		sf::Color color = castRay(start, direction, 1.5f * time, context);

		sf::Color old_color = render_image.getPixel(pixel.x, pixel.y);

		if (!use_samples) {
			const float old_conservation = 0.0f;
			mult(old_color, old_conservation);
			mult(color, 1.0f - old_conservation);
			add(old_color, color);
			render_image.setPixel(pixel.x, pixel.y, old_color);
		}
		else {
			colors[x][y].r += color.r;
			colors[x][y].g += color.g;
			colors[x][y].b += color.b;
			colors[x][y].update_count += 1.0;
		}
	}
	
	void samples_to_image()
	{
		for (uint32_t x(0); x < render_size.x; ++x) {
			for (uint32_t y(0); y < render_size.y; ++y) {
				const Sample& s = colors[x][y];
				const sf::Color color(s.r / s.update_count, s.g / s.update_count, s.b / s.update_count);
				render_image.setPixel(x, y, color);
			}
		}
	}

	void resetSamples()
	{
		for (uint32_t x(0); x < render_size.x; ++x) {
			for (uint32_t y(0); y < render_size.y; ++y) {
				Sample& s = colors[x][y];
				s.r = 0.0f;
				s.g = 0.0f;
				s.b = 0.0f;
				s.update_count = 0.0f;
			}
		}
	}

	sf::Color castRay(const glm::vec3& start, const glm::vec3& direction, float time, RayContext& context)
	{
		sf::Color result = sf::Color::Black;
		if (context.bounds > max_bounds)
			return sky_color;

		const HitPoint intersection = svo.castRay(start, direction, 128U, 0U);
		context.complexity += intersection.complexity;
		context.distance += intersection.distance;

		if (intersection.data) {
			const Cell& cell = *(intersection.data);

			if (cell.type == Cell::Solid) {
				result = getTextureColorFromHitPoint(intersection);
				if (use_ao) {
					mult(result, getAmbientOcclusion(intersection));
				}
			}
			else if (cell.type == Cell::Mirror) {
				const float roughness = 0.04f;
				const glm::vec3 normal = glm::normalize(intersection.normal + glm::vec3(roughness * getRand(), 0.0f, roughness * getRand()));
				context.bounds++;
				result = castRay(intersection.position, glm::reflect(direction, normal), time, context);
				mult(result, 0.5f);
			}
			
			const uint32_t shadow_sample = use_samples ? 4U : 1U;

			float light_intensity = shadow_sample;
			for (uint32_t i(shadow_sample); i--;) {
				const glm::vec3 light_point = light_position + glm::vec3(getRand(-25.0f, 25.0f), getRand(-25.0f, 25.0f), 0.0f);
				const glm::vec3 point_to_light = glm::normalize(light_point - intersection.position);
				const HitPoint light_intersection = svo.castRay(intersection.position, point_to_light, 128U, svo.getMaxLevel());

				if (light_intersection.data) {
					light_intensity -= 0.5f;
				}
			}

			mult(result, light_intensity / float(shadow_sample));
		}

		//const uint32_t c = std::min(255U, std::max(0U, context.complexity / 10U));
		//add(result, c);
		//return sf::Color(c, c, c);
		return result;
	}

	float getAmbientOcclusion(const HitPoint& point)
	{
		const uint32_t ray_count = 4U;
		const uint32_t max_iter = 16U;
		const glm::vec3 ao_start = point.position + point.normal * eps;
		float acc = 0.0f;
		for (uint32_t i(ray_count); i--;) {
			glm::vec3 noise_normal = glm::vec3();
	
			if (point.normal.x) {
				noise_normal = glm::vec3(0.0f, getRand(-1.0f, 1.0f), getRand(-1.0f, 1.0f));
			}
			else if (point.normal.y) {
				noise_normal = glm::vec3(getRand(-1.0f, 1.0f), 0.0f, getRand(-1.0f, 1.0f));
			}
			else if (point.normal.z) {
				noise_normal = glm::vec3(getRand(-1.0f, 1.0f), getRand(-1.0f, 1.0f), 0.0f);
			}

			HitPoint ao_point = svo.castRay(ao_start, glm::normalize(point.normal + noise_normal), max_iter, svo.getMaxLevel());
			if (!ao_point.data) {
				acc += 1.0f;
			}
		}

		return std::min(1.0f, acc / float(ray_count));
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

	std::vector<std::vector<Sample>> colors;

	sf::Image render_image;
	sf::Image image_side;
	sf::Image image_top;

	const SVO& svo;
	
	const sf::Vector2i render_size;
	sf::VertexArray& va;
	
	glm::vec3 light_position;

	sf::Color sky_color = sf::Color(119, 199, 242);

	bool use_ao = false;
	bool use_samples = false;
	const uint32_t max_bounds = 4;
	//const sf::Color sky_color = sf::Color(166, 215, 255);

	const float focal_length = 1.0f;
	const float aperture = 0.001f;
};
