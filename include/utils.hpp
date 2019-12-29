#pragma once

#include "volumetric.hpp"
#include <cmath>
#include <SFML/Graphics.hpp>


void add(sf::Color& color, float f)
{
	color.r = std::max(std::min(255.0f, color.r + f), 0.0f);
	color.g = std::max(std::min(255.0f, color.g + f), 0.0f);
	color.b = std::max(std::min(255.0f, color.b + f), 0.0f);
}


void mult(sf::Color& color, float f)
{
	color.r = std::min(255.0f, color.r * f);
	color.g = std::min(255.0f, color.g * f);
	color.b = std::min(255.0f, color.b * f);
}


float frac(float f)
{
	float whole;
	return std::modf(f, &whole);
}



