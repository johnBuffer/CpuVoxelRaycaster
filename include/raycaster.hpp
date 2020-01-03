#pragma once


#include <SFML/Graphics.hpp>
#include "volumetric.hpp"
#include "utils.hpp"


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

	void render_ray(const sf::Vector2i pixel, const glm::vec3& start, const glm::vec3& direction, float time)
	{
		const uint32_t x = pixel.x;
		const uint32_t y = pixel.y;
		va[x * render_size.y + y].position = sf::Vector2f(float(x), float(y));
		va[x * render_size.y + y].color = sky_color;

		uint32_t complexity = 0U;
		const sf::Color color = cast_ray(start, direction, 1.5f * time, 0U, complexity);

		va[x * render_size.y + y].color = color;
	}
	

	sf::Color cast_ray(const glm::vec3& start, const glm::vec3& direction, float time, uint32_t bounds, uint32_t& complexity)
	{
		if (bounds > max_bounds)
			return sky_color;

		const HitPoint intersection = volumetric.castRay(start, direction);
		complexity += intersection.complexity;
		
		if (intersection.type == Cell::Solid || intersection.type == Cell::Grass) {
			sf::Color color = getColorFromHitPoint(intersection);
			return color;
		} else if (intersection.type == Cell::Mirror || intersection.type == Cell::Water) {
			glm::vec3 normal = intersection.normal;
			if (intersection.type == Cell::Water) {
				constexpr float water_freq = 1.0f;
				glm::vec3 surface_deformation_1(sin(water_freq * intersection.position.x + time), cos(water_freq * intersection.position.x + time), 0.0f);
				glm::vec3 surface_deformation_2(sin(water_freq * intersection.position.z + time), 0.0f, cos(water_freq * intersection.position.z + time));
				normal += 0.01f * surface_deformation_1 + 0.01f * surface_deformation_2;
				normal = glm::normalize(normal);
			}

 			sf::Color color = cast_ray(intersection.position + glm::vec3(0.0f, -eps, 0.0f), glm::reflect(direction, normal), time, bounds + 1, complexity);
			const glm::vec3 light_hit = glm::normalize(intersection.position - light_position);
			const glm::vec3 light_reflection = glm::reflect(light_hit, normal);
			const bool facing_light = isFacingLight(intersection);
			float reflection_coef = facing_light ? 0.8f : 0.4f;
			
			if (intersection.type == Cell::Mirror) {
				mult(color, sf::Color(200, 220, 200));
			}
			else {
				mult(color, reflection_coef);
			}

			if (facing_light) {
				const float specular_coef = std::pow(glm::dot(light_reflection, -direction), 256);
				add(color, 255.0f * specular_coef);
			}

			//const uint32_t c = (complexity);
			//return sf::Color(c, c, c);
			return color;
		}
		else {
			return sky_color;
		}
	}

	const sf::Color getColorFromHitPoint(const HitPoint& hit_point, float color_factor = 1.0f)
	{
		sf::Color color = sky_color;
		if (hit_point.type == Cell::Solid || hit_point.type == Cell::Grass) {
			if (hit_point.type == Cell::Grass) {
				color = getColorFromVoxelCoord(getTextureFromNormal(hit_point.normal), hit_point.voxel_coord);
			}
			else {
				color = sf::Color::Red;
			}

			const float light_intensity = getLightIntensity(hit_point);
			mult(color, light_intensity);
		}
		mult(color, color_factor);

		return color;
	}

	float getLightIntensity(const HitPoint& hit_point)
	{
		constexpr float ambient_intensity = 0.4f;
		float light_intensity = ambient_intensity;

		if (isFacingLight(hit_point)) {
			const glm::vec3 hit_light = glm::normalize(light_position - hit_point.position);
			light_intensity = std::max(ambient_intensity, glm::dot(hit_point.normal, hit_light));
		}

		return light_intensity;
	}

	bool isFacingLight(const HitPoint& hit_point)
	{
		const glm::vec3 hit_light = glm::normalize(light_position - hit_point.position);
		const HitPoint light_ray = volumetric.castRay(hit_point.position + eps * hit_point.normal, hit_light);

		return light_ray.type == Cell::Empty;
	}

	const sf::Color getColorFromNormal(const glm::vec3& normal)
	{
		sf::Color color;
		if (normal.x) {
			color = sf::Color::Red;
		}
		else if (normal.y) {
			color = sf::Color::Green;
		}
		else if (normal.z) {
			color = sf::Color::Blue;
		}

		return color;
	}

	const sf::Image& getTextureFromNormal(const glm::vec3& normal)
	{
		if (normal.y) {
			return image_top;
		}

		return image_side;
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
