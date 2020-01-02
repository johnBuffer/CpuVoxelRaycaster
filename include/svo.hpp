#pragma once

#include "volumetric.hpp"
#include "utils.hpp"


struct Node
{
	Node()
	{
		for (int x(0); x < 2; ++x) {
			for (int y(0); y < 2; ++y) {
				for (int z(0); z < 2; ++z) {
					sub[x][y][z] = nullptr;
					sub_status[x][y][z] = false;
				}
			}
		}

		empty = true;
		leaf = false;
	}

	Node* sub[2][2][2];
	bool empty;
	bool leaf;
	bool sub_status[2][2][2];
	Cell cell;
};


struct Ray
{
	Ray() = default;
	
	Ray(const glm::vec3& start_, const glm::vec3& direction_)
		: start(start_)
		, direction(direction_)
		, t(glm::abs(1.0f / direction_))
		, step(direction_.x > 0.0f ? 1.0f : -1.0f, direction_.y > 0.0f ? 1.0f : -1.0f, direction_.z > 0.0f ? 1.0f : -1.0f)
		, dir(direction_.x > 0.0f ? 1.0f : 0.0f, direction_.y > 0.0f ? 1.0f : 0.0f, direction_.z > 0.0f ? 1.0f : 0.0f)
		, point()
	{
	}

	const glm::vec3 start;
	const glm::vec3 direction;
	const glm::vec3 t;
	const glm::vec3 step;
	const glm::vec3 dir;

	HitPoint point;
};


class SVO : public Volumetric
{
public:
	SVO()
	{
		m_root = new Node();
	}

	HitPoint castRay(const glm::vec3& position, const glm::vec3& direction) const override
	{
		Ray ray(position, direction);

		rec_cast_ray(ray, position, std::pow(2, m_max_level - 1), m_root);

		return ray.point;
	}

	void setCell(Cell::Type type, uint32_t x, uint32_t y, uint32_t z)
	{
		rec_setCell(type, x, y, z, m_root, std::pow(2, m_max_level));
	}

private:
	Node* m_root;
	const uint32_t m_max_level = 10U;

	void rec_setCell(Cell::Type type, uint32_t x, uint32_t y, uint32_t z, Node* node, uint32_t size)
	{
		if (!node) {
			return;
		}

		if (size == 1) {
			node->cell.type = type;
			node->leaf = true;
			return;
		}

		const uint32_t sub_size = size / 2;

		const uint32_t cell_x = x / sub_size;
		const uint32_t cell_y = y / sub_size;
		const uint32_t cell_z = z / sub_size;

		if (!node->sub[cell_x][cell_y][cell_z]) {
			node->sub[cell_x][cell_y][cell_z] = new Node();
		}
		node->sub_status[cell_x][cell_y][cell_z] = true;
		node->empty = false;

		rec_setCell(type, x - cell_x * sub_size, y - cell_y * sub_size, z - cell_z * sub_size, node->sub[cell_x][cell_y][cell_z], sub_size);
	}

	void rec_cast_ray(Ray& ray, const glm::vec3& position, uint32_t cell_size, const Node* node) const {

		glm::vec3 cell_pos_i(int(position.x / cell_size), int(position.y / cell_size), int(position.z / cell_size));
		glm::vec3 t_max = ((cell_pos_i + ray.dir) * float(cell_size) - position) * ray.t;
		uint8_t hit_side;

		float t_max_min = 0.0f;
		while (cell_pos_i.x >= 0 && cell_pos_i.y >= 0 && cell_pos_i.z >= 0 && cell_pos_i.x < 2 && cell_pos_i.y < 2 && cell_pos_i.z < 2) {
			++ray.point.complexity;

			// We enter the sub node
			if (node->sub_status[uint32_t(cell_pos_i.x)][uint32_t(cell_pos_i.y)][uint32_t(cell_pos_i.z)]) {
				const int32_t sub_size = cell_size / 2U;
				// Need to compute entry coords in its space
				// sub_position = parent_hit_coord - hit_cell_coords
				const glm::vec3 sub_position = (position + t_max_min * ray.direction) - cell_pos_i * float(cell_size);

				const Node* sub_node = node->sub[uint32_t(cell_pos_i.x)][uint32_t(cell_pos_i.y)][uint32_t(cell_pos_i.z)];

				if (sub_node->leaf) {
					const Cell& cell = sub_node->cell;
					const glm::vec3 hit = ray.start + t_max_min * ray.direction;

					HitPoint& point = ray.point;

					point.type = cell.type;
					point.position = hit;

					if (hit_side == 0) {
						point.normal = glm::vec3(-ray.step.x, 0.0f, 0.0f);
						point.voxel_coord = glm::vec2(1.0f - frac(hit.z), frac(hit.y));
					}
					else if (hit_side == 1) {
						point.normal = glm::vec3(0.0f, -ray.step.y, 0.0f);
						point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.z));
					}
					else if (hit_side == 2) {
						point.normal = glm::vec3(0.0f, 0.0f, -ray.step.z);
						point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.y));
					}

					point.distance = t_max_min;
					return;
				}
				else {
					rec_cast_ray(ray, sub_position, cell_size / 2, sub_node);
				}
			}

			if (t_max.x < t_max.y) {
				if (t_max.x < t_max.z) {
					t_max_min = t_max.x;
					t_max.x += ray.t.x * cell_size, cell_pos_i.x += ray.step.x, hit_side = 0;
				}
				else {
					t_max_min = t_max.z;
					t_max.z += ray.t.z * cell_size, cell_pos_i.z += ray.step.z, hit_side = 2;
				}
			}
			else {
				if (t_max.y < t_max.z) {
					t_max_min = t_max.y;
					t_max.y += ray.t.y * cell_size, cell_pos_i.y += ray.step.y, hit_side = 1;
				}
				else {
					t_max_min = t_max.z;
					t_max.z += ray.t.z * cell_size, cell_pos_i.z += ray.step.z, hit_side = 2;
				}
			}
		}
	}
};
