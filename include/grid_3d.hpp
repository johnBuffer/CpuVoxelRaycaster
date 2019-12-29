#pragma once

#include <stdint.h>
#include "volumetric.hpp"
#include <array>


struct Cell
{
	Cell()
		: empty(true)
	{}

	bool empty;
};


template<int32_t X, int32_t Y, int32_t Z>
class Grid3D : public Volumetric
{
public:
	Grid3D(uint32_t cell_size=1);

	HitPoint cast_ray(const glm::vec3& position, const glm::vec3& direction) const override;

	void setCell(bool empty, uint32_t x, uint32_t y, uint32_t z);

private:
	const uint32_t m_cell_size;
	std::array<std::array<std::array<Cell, Z>, Y>, X> m_cells;
};

template<int32_t X, int32_t Y, int32_t Z>
inline Grid3D<X, Y, Z>::Grid3D(uint32_t cell_size)
	: m_cell_size(cell_size)
{
	constexpr uint64_t size = X * Y * Z;
}

template<int32_t X, int32_t Y, int32_t Z>
inline HitPoint Grid3D<X, Y, Z>::cast_ray(const glm::vec3& position, const glm::vec3& direction) const
{
	HitPoint point;
	// Initialization
	// We assume we have a ray vector:
	// vec = start + t*v

	// cell_x and cell_y are the starting voxel's coordinates
	int32_t cell_x = position.x / m_cell_size;
	int32_t cell_y = position.y / m_cell_size;
	int32_t cell_z = position.z / m_cell_size;

	// step_x and step_y describe if cell_x and cell_y
	// are incremented or decremented during iterations
	const int32_t step_x = direction.x < 0 ? -1 : 1;
	const int32_t step_y = direction.y < 0 ? -1 : 1;
	const int32_t step_z = direction.z < 0 ? -1 : 1;

	// Compute the value of t for first intersection in x and y
	const int32_t dir_x = step_x > 0 ? 1 : 0;
	float t_max_x = ((cell_x + dir_x)*m_cell_size - position.x) / direction.x;
	const int32_t dir_y = step_y > 0 ? 1 : 0;
	float t_max_y = ((cell_y + dir_y)*m_cell_size - position.y) / direction.y;
	const int32_t dir_z = step_z > 0 ? 1 : 0;
	float t_max_z = ((cell_z + dir_z)*m_cell_size - position.z) / direction.z;

	// Compute how much (in units of t) we can move along the ray
	// before reaching the cell's width and height
	float t_dx = std::abs(float(m_cell_size) / direction.x);
	float t_dy = std::abs(float(m_cell_size) / direction.y);
	float t_dz = std::abs(float(m_cell_size) / direction.z);

	uint8_t hit_side;

	while (cell_x >= 0 && cell_y >= 0 && cell_z >= 0 && cell_x < X && cell_y < Y && cell_z < Z)
	{
		float t_max_min;
		if (t_max_x < t_max_y) {
			if (t_max_x < t_max_z) {
				t_max_min = t_max_x;
				t_max_x += t_dx;
				cell_x += step_x;
				hit_side = 0;
			}
			else {
				t_max_min = t_max_z;
				t_max_z += t_dz;
				cell_z += step_z;
				hit_side = 2;
			}
		}
		else {
			if (t_max_y < t_max_z) {
				t_max_min = t_max_y;
				t_max_y += t_dy;
				cell_y += step_y;
				hit_side = 1;
			}
			else {
				t_max_min = t_max_z;
				t_max_z += t_dz;
				cell_z += step_z;
				hit_side = 2;
			}
		}

		if (cell_x >= 0 && cell_y >= 0 && cell_z >= 0 && cell_x < X && cell_y < Y && cell_z < Z) {
			if (!m_cells[cell_x][cell_y][cell_z].empty) {
				float hit_x = position.x + t_max_min * direction.x;
				float hit_y = position.y + t_max_min * direction.y;
				float hit_z = position.z + t_max_min * direction.z;

				point.hit = true, point.light_intensity = 0.5f * hit_side;
				point.position = glm::vec3(hit_x, hit_y, hit_z);

				if (hit_side == 0) {
					point.normal = glm::vec3(-step_x, 0.0f, 0.0f);
				} else if (hit_side == 1) {
					point.normal = glm::vec3(0.0f, -step_y, 0.0f);
				}else if (hit_side == 2) {
					point.normal = glm::vec3(0.0f, 0.0f, -step_z);
				}

				break;
			}
		}
	}

	return point;
}

template<int32_t X, int32_t Y, int32_t Z>
inline void Grid3D<X, Y, Z>::setCell(bool empty, uint32_t x, uint32_t y, uint32_t z)
{
	m_cells[x][y][z].empty = empty;
}
