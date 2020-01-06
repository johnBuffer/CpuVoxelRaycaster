#pragma once

#include <cstdint>


struct Vec3f
{
	Vec3f() : Vec3f(0.0f, 0.0f, 0.0f) {}

	Vec3f(float x_, float y_, float z_)
		: x(x_)
		, y(y_)
		, z(z_)
	{}
	float x, y, z;
};

struct Vec3i
{
	Vec3i() : Vec3i(0, 0, 0) {}

	Vec3i(int32_t x_, int32_t y_, int32_t z_)
		: x(x_)
		, y(y_)
		, z(z_)
	{}

	int32_t x, y, z;
};
