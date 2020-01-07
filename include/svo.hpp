#pragma once

#include "volumetric.hpp"
#include "utils.hpp"

struct MipmapCell
{
	bool empty;
	Cell* data;
};

struct MipmapGrid
{
	MipmapGrid() = default;
	MipmapGrid(const uint32_t level, const uint32_t level_count)
		// The level N (starting at 0) has a size of 2^(N+1)
		: size(uint32_t(std::pow(2, level + 1)))
		, cell_size(uint32_t(std::pow(2, level_count) / std::pow(2, level + 1)))
		, leaf_level(level == level_count - 1)
	{
		cells.resize(size * size * size);
		for (MipmapCell& cell : cells) {
			cell.empty = true;
			cell.data = nullptr;
		}
	}

	MipmapCell& get(const uint32_t x, const uint32_t y, const uint32_t z)
	{
		return cells[z * size * size + y * size + x];
	}

	MipmapCell& get(const glm::ivec3& coords)
	{
		return get(coords.x, coords.y, coords.z);
	}

	MipmapCell& get(const glm::vec3& coords)
	{
		return get(uint32_t(coords.x), uint32_t(coords.y), uint32_t(coords.z));
	}

	const MipmapCell& get(const uint32_t x, const uint32_t y, const uint32_t z) const
	{
		return cells[z * size * size + y * size + x];
	}

	const MipmapCell& get(const glm::ivec3& coords) const
	{
		return get(uint32_t(coords.x), uint32_t(coords.y), uint32_t(coords.z));
	}

	const MipmapCell& get(const glm::vec3& coords) const
	{
		return get(uint32_t(coords.x), uint32_t(coords.y), uint32_t(coords.z));
	}

	const uint32_t size;
	const uint32_t cell_size;

	const bool leaf_level;
	std::vector<MipmapCell> cells;
};

struct Ray
{
	Ray() = default;
	
	Ray(const glm::vec3& start_, const glm::vec3& direction_)
		: start(start_)
		, direction(direction_)
		, inv_direction(1.0f / direction_)
		, t(glm::abs(1.0f / direction_))
		, step(direction_.x >= 0.0f ? 1.0f : -1.0f, copysign(1.0f, direction_.y), copysign(1.0f, direction_.z))
		, dir(direction_.x > 0.0f, direction_.y > 0.0f, direction_.z > 0.0f)
		, point()
		, t_total(0.0f)
		, hit_side(0U)
	{
	}

	const glm::vec3 start;
	const glm::vec3 direction;
	const glm::vec3 inv_direction;
	const glm::vec3 t;
	const glm::vec3 step;
	const glm::vec3 dir;
	uint8_t hit_side;
	float t_total;

	HitPoint point;
};


class SVO
{
public:
	SVO()
	{
		for (uint32_t level(0U); level < m_level_count; ++level) {
			m_data.emplace_back(level, m_level_count);
		}
	}

	HitPoint castRay(const glm::vec3& position, const glm::vec3& direction, const uint32_t max_iter, const uint32_t start_level) const
	{
		Ray ray(position, direction);

		stack_castRay(ray, position, max_iter, start_level);

		return ray.point;
	}

	void setCell(Cell::Type type, Cell::Texture texture, uint32_t x, uint32_t y, uint32_t z)
	{
		for (uint32_t level(0U); level < m_level_count; ++level) {
			setCellMipmap(type, texture, x, y, z, level);
		}
	}

	static bool checkCell(const glm::vec3& cell_coords)
	{
		return cell_coords.x >= 0 &&
			   cell_coords.y >= 0 &&
			   cell_coords.z >= 0 &&
			   cell_coords.x < 2 &&
			   cell_coords.y < 2 &&
			   cell_coords.z < 2;
	}

	bool checkCell(const glm::vec3& cell_coords, const uint32_t level) const
	{
		const uint32_t grid_size = m_data[level].size;
		return cell_coords.x >= 0 &&
			cell_coords.y >= 0 &&
			cell_coords.z >= 0 &&
			cell_coords.x < grid_size &&
			cell_coords.y < grid_size &&
			cell_coords.z < grid_size;
	}

	const uint32_t getMaxLevel() const
	{
		return m_level_count - 1U;
	}

private:
	std::vector<MipmapGrid> m_data;
	const uint32_t m_level_count = 8U;

	void setCellMipmap(Cell::Type type, Cell::Texture texture, uint32_t x, uint32_t y, uint32_t z, const uint32_t level)
	{
		MipmapGrid& current_grid = m_data[level];
		if (current_grid.leaf_level) {
			MipmapCell& cell = current_grid.get(x, y, z);
			if (!cell.data) {
				cell.data = new Cell();
			}

			cell.data->type = type;
			cell.data->texture = texture;
			cell.empty = false;
		}
		else {
			const uint32_t cell_size = current_grid.cell_size;
			const uint32_t cell_x = x / cell_size;
			const uint32_t cell_y = y / cell_size;
			const uint32_t cell_z = z / cell_size;

			current_grid.get(cell_x, cell_y, cell_z).empty = false;
		}
	}

	void fillHitResult(Ray& ray, const MipmapCell& node, const float t) const
	{
		HitPoint& point = ray.point;
		const Cell& data = *node.data;
		const glm::vec3 hit = ray.start + t * ray.direction;

		point.data = &data;
		point.position = hit;
		point.distance = t;

		if (ray.hit_side == 0) {
			point.normal = glm::vec3(-ray.step.x, 0.0f, 0.0f);
			point.voxel_coord = glm::vec2(1.0f - frac(hit.z), frac(hit.y));
		}
		else if (ray.hit_side == 1) {
			point.normal = glm::vec3(0.0f, -ray.step.y, 0.0f);
			point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.z));
		}
		else if (ray.hit_side == 2) {
			point.normal = glm::vec3(0.0f, 0.0f, -ray.step.z);
			point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.y));
		}
	}

	void stack_castRay(Ray& ray, const glm::vec3& start_position, const uint32_t max_iter, const uint32_t start_level) const
	{
		int32_t level = start_level;
		glm::vec3 position = start_position;		

		while (level > -1 && ray.point.complexity < max_iter) {
			const MipmapGrid& current_level = m_data[level];
			const uint32_t cell_size = current_level.cell_size;
			const uint32_t top_size = current_level.size  * 2U;
			
			const glm::ivec3 cell_position_i = glm::ivec3(position.x / cell_size, position.y / cell_size, position.z / cell_size);
			glm::vec3 cell_position = cell_position_i;
			glm::vec3 cell_start_position(cell_position_i.x, cell_position_i.y, cell_position_i.z);
			glm::vec3 t_max = ((cell_position + ray.dir) * float(cell_size) - position) / ray.direction;

			const glm::vec3 t = float(cell_size) * ray.t;

			float t_max_min = 0.0f;
			const float t_total = ray.t_total;

			while (checkCell(cell_position, level) && ray.point.complexity < max_iter) {
				// Increase pixel complexity
				++ray.point.complexity;

				if (!checkCell(cell_position - cell_start_position)) {
					--level;
					break;
				}

				// We enter the sub node
				const MipmapCell& sub_node = current_level.get(cell_position_i);
				if (!sub_node.empty) {
					if (current_level.leaf_level) {
						fillHitResult(ray, sub_node, t_total + t_max_min);
					}
					else {
						++level;
						break;
					}
				}

				const uint32_t min_comp = getMinComponentIndex(t_max);
				t_max_min = t_max[min_comp];
				t_max[min_comp] += t[min_comp];
				cell_position += ray.step[min_comp];
				ray.hit_side = min_comp;
			}

			position += t_max_min * ray.direction;
			ray.t_total = t_total + t_max_min;
		}
	}

	void rec_castRay(Ray& ray, const glm::vec3& position, const uint32_t level) const
	{
		const MipmapGrid& current_level = m_data[level];
		const uint32_t cell_size = current_level.cell_size;

		glm::vec3 cell_pos_i = glm::ivec3(position.x / cell_size, position.y / cell_size, position.z / cell_size);
		glm::vec3 t_max = ((cell_pos_i + ray.dir) * float(cell_size) - position) / ray.direction;

		const glm::vec3 t = float(cell_size) * ray.t;

		float t_max_min = 0.0f;
		const float t_total = ray.t_total;
		while (checkCell(cell_pos_i, level)) {
			// Increase pixel complexity
			++ray.point.complexity;
			// We enter the sub node
			const MipmapCell& sub_node = current_level.get(cell_pos_i);
			if (!sub_node.empty) {
				if (current_level.leaf_level) {
					HitPoint& point = ray.point;
					const Cell& data = *sub_node.data;
					const glm::vec3 hit = ray.start + (t_total + t_max_min) * ray.direction;

					point.data = &data;
					point.position = hit;

					if (ray.hit_side == 0) {
						point.normal = glm::vec3(-ray.step.x, 0.0f, 0.0f);
						point.voxel_coord = glm::vec2(1.0f - frac(hit.z), frac(hit.y));
					}
					else if (ray.hit_side == 1) {
						point.normal = glm::vec3(0.0f, -ray.step.y, 0.0f);
						point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.z));
					}
					else if (ray.hit_side == 2) {
						point.normal = glm::vec3(0.0f, 0.0f, -ray.step.z);
						point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.y));
					}

					point.distance = t_total + t_max_min;
					return;
				}
				else {
					const glm::vec3 sub_position = position + t_max_min * ray.direction;
					ray.t_total = t_total + t_max_min;
					rec_castRay(ray, sub_position, level+1U);
					if (ray.point.data) {
						return;
					}
				}
			}

			if (t_max.x < t_max.y) {
				if (t_max.x < t_max.z) {
					t_max_min = t_max.x;
					t_max.x += t.x, cell_pos_i.x += ray.step.x, ray.hit_side = 0;
				}
				else {
					t_max_min = t_max.z;
					t_max.z += t.z, cell_pos_i.z += ray.step.z, ray.hit_side = 2;
				}
			}
			else {
				if (t_max.y < t_max.z) {
					t_max_min = t_max.y;
					t_max.y += t.y, cell_pos_i.y += ray.step.y, ray.hit_side = 1;
				}
				else {
					t_max_min = t_max.z;
					t_max.z += t.z, cell_pos_i.z += ray.step.z, ray.hit_side = 2;
				}
			}
		}
	}
};
