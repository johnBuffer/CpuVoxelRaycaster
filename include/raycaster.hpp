#pragma once


#include <SFML/Graphics.hpp>
#include "lsvo.hpp"
#include "utils.hpp"


struct RayContext
{
	float distance = 0.0f;
	uint32_t complexity = 0U;
	uint32_t bounds = 0U;
	int32_t gi_bounce = 2U;
};


struct Sample
{
	double update_count = 0.0f;
	double r = 0.0f;
	double g = 0.0f;
	double b = 0.0f;
};


struct GIContribution
{
	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float intensity = 0.0f;
	float n_points;
};

struct ColorResult
{
	sf::Color color = sf::Color::Black;
	float distance = 0.0f;
};

constexpr uint8_t SVO_DEPTH = 9u;
struct RayCaster
{
	const float eps = 0.001f;
	const float sun_intensity = 1000000.0f;

	RayCaster(const LSVO<SVO_DEPTH>& svo_, const sf::Vector2i& render_size_)
		: svo(svo_)
		, render_size(render_size_)
	{
		render_image.create(render_size.x, render_size.y);
		image_side.loadFromFile("res/grass_side_16x16.bmp");
		image_top.loadFromFile("res/grass_top_16x16.bmp");

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

		ColorResult result = castRay(start, direction, 1.5f * time, context);

		sf::Color old_color = render_image.getPixel(pixel.x, pixel.y);

		if (!use_samples) {
			const float old_conservation = 0.4f;
			mult(old_color, old_conservation);
			mult(result.color, 1.0f - old_conservation);
			add(old_color, result.color);
			render_image.setPixel(pixel.x, pixel.y, old_color);
		}
		else {
			colors[x][y].r += result.color.r;
			colors[x][y].g += result.color.g;
			colors[x][y].b += result.color.b;
			colors[x][y].update_count += 1.0;
		}
	}

	void samples_to_image()
	{
		for (int32_t x(0); x < render_size.x; ++x) {
			for (int32_t y(0); y < render_size.y; ++y) {
				const Sample& s = colors[x][y];
				const sf::Color color(s.r / s.update_count, s.g / s.update_count, s.b / s.update_count);
				render_image.setPixel(x, y, color);
			}
		}
	}

	void resetSamples()
	{
		for (int32_t x(0); x < render_size.x; ++x) {
			for (int32_t y(0); y < render_size.y; ++y) {
				Sample& s = colors[x][y];
				s.r = 0.0f;
				s.g = 0.0f;
				s.b = 0.0f;
				s.update_count = 0.0f;
			}
		}
	}

	ColorResult castRay(const glm::vec3& start, const glm::vec3& direction, float time, RayContext& context)
	{
		// Const values
		constexpr uint8_t SVO_MAX_DEPTH = 23u;
		constexpr uint8_t DEPTH_OFFSET = SVO_MAX_DEPTH - SVO_DEPTH;
		constexpr float SVO_SIZE = 1 << SVO_DEPTH;
		constexpr float SCALE = 1.0f / SVO_SIZE;
		constexpr float epsilon = 1.0f / float(1 << SVO_MAX_DEPTH);
		ColorResult result;
		if (context.bounds > max_bounds) {
			return result;
		}

		const HitPoint intersection = svo.castRay(start, direction, 0.0f, 0.0f);
		context.complexity += intersection.complexity;
		context.distance = intersection.distance;

		if (intersection.cell) {
			const glm::vec3& normal = intersection.normal;
			result.distance = intersection.distance;
			const Cell& cell = *(intersection.cell);
			const glm::vec3 hit_position = intersection.position + normal * SCALE * 0.001f;

			sf::Color albedo = sf::Color::White;
			if (cell.type == Cell::Solid) {
				albedo = getTextureColorFromHitPoint(intersection);
				result.color = albedo;
			}

			const uint32_t shadow_sample = use_samples ? 4U : 1U;
			float light_intensity = 0.0f;
			if (cell.texture != Cell::Red) {
				for (uint32_t i(shadow_sample); i--;) {
					const glm::vec3 light_point = light_position;// +glm::vec3(getRand(-25.0f, 25.0f), getRand(-25.0f, 25.0f), 0.0f);
					const glm::vec3 point_to_light = glm::normalize(light_point - hit_position);
					const HitPoint light_intersection = svo.castRay(hit_position, point_to_light);

					if (!light_intersection.cell) {
						light_intensity = std::max(0.0f, glm::dot(point_to_light, normal));
					}
				}
			}

			const float gi_intensity = use_gi ? getGlobalIllumination(intersection) : 0.0f;

			mult(result.color, std::min(1.0f, std::max(0.0f, light_intensity + gi_intensity)));
		}

		return result;
	}

	float getGlobalIllumination(const HitPoint& point)
	{
		constexpr float SCALE = 1.0f / 512.0f;
		constexpr float n_normalizer = SCALE * 0.0078125f * 2.0f;
		constexpr uint32_t ray_count = 1U;
		const glm::vec3 gi_start = point.position + point.normal * n_normalizer;
		float acc = 0.0f;
		const glm::vec3& normal = point.normal;
		constexpr float range = 1000.0f;
		for (uint32_t i(ray_count); i--;) {
			glm::vec3 noise_normal;
			const float coord_1 = getRand(-range, range);
			const float coord_2 = getRand(-range, range);
			if (normal.x) {
				noise_normal = glm::vec3(0.0f, coord_1, coord_2);
			}
			else if (normal.y) {
				noise_normal = glm::vec3(coord_1, 0.0f, coord_2);
			}
			else if (normal.z) {
				noise_normal = glm::vec3(coord_1, coord_2, 0.0f);
			}

			const glm::vec3 gi_ray = glm::normalize((normal + noise_normal) * n_normalizer);
			const float dot_gi = glm::dot(gi_ray, point.normal);
			const HitPoint gi_point = svo.castRay(gi_start, gi_ray, 0.5f, 0.0f);
			if (gi_point.cell) {
				const glm::vec3 gi_light_start = gi_point.position + gi_point.normal * n_normalizer;
				const glm::vec3 to_light = glm::normalize(light_position - gi_light_start);
				const HitPoint gi_light_point = svo.castRay(gi_light_start, to_light, 0.5f, 0.0f);
				if (!gi_light_point.cell) {
					const float dot = glm::dot(gi_point.normal, to_light);
					acc += sun_intensity * std::min(0.5f, std::max(0.0f, dot) * dot_gi);
				}
			}
		}

		return std::max(0.0f, acc / float(ray_count));
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
		else if (point.cell->texture == Cell::Red) {
			return sf::Color::Red;
		}
		else if (point.cell->texture == Cell::White) {
			return sf::Color::White;
		}
		else {
			return sf::Color::Magenta;
		}
	}

	const sf::Color getColorFromVoxelCoord(const sf::Image& image, glm::vec2 coords)
	{
		const sf::Vector2u tex_size = image.getSize();
		clamp(coords.x, 0.0f, 1.0f);
		clamp(coords.y, 0.0f, 1.0f);
		return image.getPixel(uint32_t(tex_size.x * coords.x), uint32_t(tex_size.y * coords.y));
	}

	const sf::Color getColorFromNormal(const glm::vec3& normal)
	{
		//return sf::Color::White;
		if (normal.x) {
			return sf::Color::Red;
		}
		else if (normal.y) {
			return sf::Color::Green;
		}
		else if (normal.z) {
			return sf::Color::Blue;
		}

		//return sf::Color(normal.x * 255, normal.y * 255, normal.z * 255);
		//return sf::Color::Black;
	}

	std::vector<std::vector<Sample>> colors;

	sf::Image render_image;
	sf::Image image_side;
	sf::Image image_top;

	const LSVO<SVO_DEPTH>& svo;

	const sf::Vector2i render_size;

	glm::vec3 light_position;

	sf::Color sky_color = sf::Color(119, 199, 242);

	bool use_ao = false;
	bool use_gi = false;
	bool use_samples = false;
	bool use_god_rays = false;
	const uint32_t max_bounds = 4;
	//const sf::Color sky_color = sf::Color(166, 215, 255);


	const float focal_length = 1.0f;
	const float aperture = 0.001f;
};