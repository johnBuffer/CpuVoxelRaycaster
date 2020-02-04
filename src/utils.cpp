#pragma once

#include "utils.hpp"
#include <cmath>
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>


void add(sf::Color& color, float f)
{
	color.r = std::uint8_t(std::max(std::min(255.0f, color.r + f), 0.0f));
	color.g = std::uint8_t(std::max(std::min(255.0f, color.g + f), 0.0f));
	color.b = std::uint8_t(std::max(std::min(255.0f, color.b + f), 0.0f));
}

void add(sf::Color& color1, const sf::Color& color2)
{
	color1.r = std::uint8_t(std::max(std::min(255, color1.r + color2.r), 0));
	color1.g = std::uint8_t(std::max(std::min(255, color1.g + color2.g), 0));
	color1.b = std::uint8_t(std::max(std::min(255, color1.b + color2.b), 0));
}


void mult(sf::Color& color, float f)
{
	color.r = std::uint8_t(std::min(255.0f, color.r * f));
	color.g = std::uint8_t(std::min(255.0f, color.g * f));
	color.b = std::uint8_t(std::min(255.0f, color.b * f));
}


void mult(sf::Color& color1, const sf::Color& color2)
{
	constexpr float inv = 1.0f / 255.0f;
	color1.r = std::uint8_t(color1.r * color2.r * inv);
	color1.g = std::uint8_t(color1.g * color2.g * inv);
	color1.b = std::uint8_t(color1.b * color2.b * inv);
}


float frac(float f)
{
	float whole;
	return std::modf(f, &whole);
}


void clamp(float& value, float min, float max)
{
	if (value > max) {
		value = max;
	}
	else if (value < min) {
		value = min;
	}
}

float getRand(float min, float max)
{
	const float rand_val = float(rand() % 100) / 100.0f;
	return min + (max - min) * rand_val;
}

const uint32_t getMinComponentIndex(const glm::vec3& v)
{
	if (v.x < v.y) {
		return v.x < v.z ? 0U : 2U;
	}
	else if (v.y < v.z) {
		return 1U;
	}
	return 2U;
}

glm::mat3 generateRotationMatrix(const glm::vec2& angle)
{
	const glm::mat4 rx = glm::rotate(glm::mat4(1.0f), -angle.x, glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4 ry = glm::rotate(glm::mat4(1.0f), -angle.y, glm::vec3(1.0f, 0.0f, 0.0f));

	return ry * rx;
}

std::string toString(const glm::vec3& vec)
{
	std::stringstream sx;
	sx << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
	return sx.str();
}
