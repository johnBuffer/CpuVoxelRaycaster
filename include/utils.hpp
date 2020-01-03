#pragma once

#include "volumetric.hpp"
#include <cmath>
#include <SFML/Graphics.hpp>


void add(sf::Color& color, float f)
{
	color.r = std::uint8_t(std::max(std::min(255.0f, color.r + f), 0.0f));
	color.g = std::uint8_t(std::max(std::min(255.0f, color.g + f), 0.0f));
	color.b = std::uint8_t(std::max(std::min(255.0f, color.b + f), 0.0f));
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
