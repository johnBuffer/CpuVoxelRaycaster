#pragma once

#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>


void add(sf::Color& color, float f);

void add(sf::Color& color1, const sf::Color& color2);

void mult(sf::Color& color, float f);

void mult(sf::Color& color1, const sf::Color& color2);

float frac(float f);

void clamp(float& value, float min, float max);

float getRand(float min = -0.5f, float max = 0.5f);

const uint32_t getMinComponentIndex(const glm::vec3& v);

glm::mat3 generateRotationMatrix(const glm::vec2& angle);

std::string toString(const glm::vec3& vec);

uint32_t floatAsInt(float f);

float intAsFloat(uint32_t i);
