#pragma once

#include <stdint.h>
#include "volumetric.hpp"
#include <array>
#include "cell.hpp"
#include "utils.hpp"


template<int32_t X, int32_t Y, int32_t Z>
class Grid3D_packed : public Volumetric
{
public:
	Grid3D_packed();

	HitPoint cast_ray(const glm::vec3& position, const glm::vec3& direction) const override;

	void setCell(Cell::Type type, uint32_t x, uint32_t y, uint32_t z);

	const Cell& getCell_const(int32_t x, int32_t y, int32_t z) const;
	Cell& getCell(int32_t x, int32_t y, int32_t z);

private:
	std::array<Cell, X*Y*Z> m_cells;

	const uint64_t size = X * Y * Z;
	const uint32_t chunk_size = 1U;
	const uint32_t chunk_elements = chunk_size * chunk_size * chunk_size;
	const uint32_t grid_chunk_width  = X / chunk_size;
	const uint32_t grid_chunk_height = Y / chunk_size;
	const uint32_t grid_chunk_depth  = Z / chunk_size;
};

template<int32_t X, int32_t Y, int32_t Z>
inline Grid3D_packed<X, Y, Z>::Grid3D_packed()
{
}

template<int32_t X, int32_t Y, int32_t Z>
inline HitPoint Grid3D_packed<X, Y, Z>::cast_ray(const glm::vec3& position, const glm::vec3& direction) const
{
	HitPoint point;

	// Compute how much (in units of t) we can move along the ray
	// before reaching the cell's width and height
	const float t_dx = std::abs(1.0f / direction.x);
	const float t_dy = std::abs(1.0f / direction.y);
	const float t_dz = std::abs(1.0f / direction.z);

	// step_x and step_y describe if cell_x and cell_y
	// are incremented or decremented during iterations
	const int32_t step_x = direction.x < 0 ? -1 : 1;
	const int32_t step_y = direction.y < 0 ? -1 : 1;
	const int32_t step_z = direction.z < 0 ? -1 : 1;

	// Compute the value of t for first intersection in x and y
	const int32_t dir_x = step_x > 0 ? 1 : 0;
	const int32_t dir_y = step_y > 0 ? 1 : 0;
	const int32_t dir_z = step_z > 0 ? 1 : 0;

	// cell_x and cell_y are the starting voxel's coordinates
	int32_t cell_x = position.x;
	int32_t cell_y = position.y;
	int32_t cell_z = position.z;

	float t_max_x = ((cell_x + dir_x) - position.x) / direction.x;
	float t_max_y = ((cell_y + dir_y) - position.y) / direction.y;
	float t_max_z = ((cell_z + dir_z) - position.z) / direction.z;

	uint8_t hit_side;

	while (cell_x >= 0 && cell_y >= 0 && cell_z >= 0 && cell_x < X && cell_y < Y && cell_z < Z) {
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
			const Cell& cell = getCell_const(cell_x, cell_y, cell_z);
			if (cell.type != Cell::Empty) {
				float hit_x = position.x + t_max_min * direction.x;
				float hit_y = position.y + t_max_min * direction.y;
				float hit_z = position.z + t_max_min * direction.z;

				point.type = cell.type;
				point.position = glm::vec3(hit_x, hit_y, hit_z);

				if (hit_side == 0) {
					point.normal = glm::vec3(-step_x, 0.0f, 0.0f);
					point.voxel_coord = glm::vec2(1.0f-frac(hit_z), frac(hit_y));
				} else if (hit_side == 1) {
					point.normal = glm::vec3(0.0f, -step_y, 0.0f);
					point.voxel_coord = glm::vec2(frac(hit_x), frac(hit_z));
				}else if (hit_side == 2) {
					point.normal = glm::vec3(0.0f, 0.0f, -step_z);
					point.voxel_coord = glm::vec2(frac(hit_x), frac(hit_y));
				}

				point.distance = t_max_min;

				break;
			}
		}
	}

	return point;
}

template<int32_t X, int32_t Y, int32_t Z>
inline void Grid3D_packed<X, Y, Z>::setCell(Cell::Type type, uint32_t x, uint32_t y, uint32_t z)
{
	getCell(x, y, z).type = type;
}

template<int32_t X, int32_t Y, int32_t Z>
inline const Cell& Grid3D_packed<X, Y, Z>::getCell_const(int32_t x, int32_t y, int32_t z) const
{
	const int32_t chunk_x = x / chunk_size;
	const int32_t chunk_y = y / chunk_size;
	const int32_t chunk_z = z / chunk_size;

	const int32_t in_chunk_x = x % chunk_size;
	const int32_t in_chunk_y = y % chunk_size;
	const int32_t in_chunk_z = z % chunk_size;

	const uint32_t chunk_index = grid_chunk_width * grid_chunk_height * chunk_z + chunk_y * grid_chunk_width + chunk_x;
	const uint32_t cell_index = chunk_index * chunk_elements + in_chunk_z * chunk_size * chunk_size + in_chunk_y * chunk_size + in_chunk_x;

	return m_cells[cell_index];
}

template<int32_t X, int32_t Y, int32_t Z>
inline Cell& Grid3D_packed<X, Y, Z>::getCell(int32_t x, int32_t y, int32_t z)
{
	return const_cast<Cell&>(getCell_const(x, y, z));
}
