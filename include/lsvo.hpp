#pragma once

#include "svo.hpp"
#include "volumetric.hpp"
#include <bitset>


struct LNode
{
	LNode()
		: child_mask(0U)
		, leaf_mask(0U)
		, child_offset(0U)
	{}

	uint8_t  child_mask;
	uint8_t  leaf_mask;
	uint32_t child_offset;
};


void compileSVO_rec(const Node* node, std::vector<LNode>& data, const uint32_t node_index, uint32_t& max_offset)
{
	if (node) {
		const uint32_t child_pos = data.size();
		const uint32_t offset = child_pos - node_index;
		max_offset = offset > max_offset ? offset : max_offset;
		data[node_index].child_offset = offset;

		bool empty = true;
		for (uint8_t x(0U); x < 2; ++x) {
			for (uint8_t y(0U); y < 2; ++y) {
				for (uint8_t z(0U); z < 2; ++z) {
					if (node->sub[x][y][z]) {
						empty = false;
						break;
					}
				}
			}
		}

		if (!empty) {
			for (uint8_t i(8U); i--;) {
				data.emplace_back();
			}

			for (uint8_t x(0U); x < 2; ++x) {
				for (uint8_t y(0U); y < 2; ++y) {
					for (uint8_t z(0U); z < 2; ++z) {
						const Node* sub_node = node->sub[x][y][z];
						if (sub_node) {
							const uint8_t sub_index = z * 4 + y * 2 + x;
							data[node_index].child_mask |= (1U << sub_index);
							// std::cout << "Add child to IDX " << node_index << " Child Mask " << std::bitset<8>(data[node_index].child_mask) << std::endl;
							if (!(sub_node->leaf)) {
								compileSVO_rec(sub_node, data, child_pos + sub_index, max_offset);
							}
							else {
								data[node_index].leaf_mask |= (1U << sub_index);
							}
						}
					}
				}
			}
		}
	}
}


std::vector<LNode> compileSVO(const SVO& svo)
{
	std::vector<LNode> data;
	data.push_back(LNode());

	uint32_t max_offset = 0U;
	compileSVO_rec(svo.m_root, data, 0, max_offset);
	std::cout << "MAX OFFSET: " << max_offset << std::endl;

	return data;
}


struct LSVO : public Volumetric
{
	void importFromSVO(const SVO& svo)
	{
		data = compileSVO(svo);
		max_level = svo.m_max_level;

		cell = new Cell();
		cell->type = Cell::Type::Solid;
		cell->texture = Cell::Texture::Grass;
	}

	void print() const
	{
		std::cout << std::endl;
		print_rec(0, "");
	}

	void print_rec(const uint32_t node_index, const std::string level_indent) const
	{
		const std::string indent = "  ";
		LNode current_node = data[node_index];
		std::cout << level_indent << "Index " << node_index << " offset " << current_node.child_offset << " leaf_mask " << std::bitset<8>(current_node.leaf_mask) << " child_mask " << std::bitset<8>(current_node.child_mask);
		for (uint32_t i(0); i < 8U; ++i) {
			if (hasChild(current_node.child_mask, i)) {
				std::cout << " (" << i % 2 << ", " << (i / 2U)%2 << ", " << i / 4U << ")";
			}
		}
		std::cout << std::endl;

		for (uint32_t i(0); i < 8U; ++i) {
			const uint32_t child_index = node_index + current_node.child_offset + i;
			if (hasChild(current_node.child_mask, i)) {
				if (isLeaf(current_node.leaf_mask, i)) {
					std::cout << level_indent + indent << "Index " << child_index << " LEAF (" << i % 2 << ", " << (i / 2U)%2 << ", " << i / 4U << ")" << std::endl;
				}
				else {
					print_rec(child_index, level_indent + indent);
				}
			}
			else {
				std::cout << level_indent + indent << "Index " << child_index << " EMPTY" << std::endl;
			}
		}
	}

	void setCell(Cell::Type type, Cell::Texture texture, uint32_t x, uint32_t y, uint32_t z) {}

	HitPoint castRay(const glm::vec3& position, const glm::vec3& direction, const uint32_t max_iter) const
	{
		Ray ray(position, direction);

		const uint32_t max_cell_size = uint32_t(std::pow(2, max_level-1));
		rec_castRay(ray, position, max_cell_size, 0, max_iter);

		return ray.point;
	}

	void fillHitResult(Ray& ray, const float t) const
	{
		HitPoint& point = ray.point;
		const glm::vec3 hit = ray.start + t * ray.direction;

		point.cell = cell;
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

	inline static bool checkCell(const glm::vec3& cell_coords)
	{
		return cell_coords.x >= 0 &&
			cell_coords.y >= 0 &&
			cell_coords.z >= 0 &&
			cell_coords.x < 2 &&
			cell_coords.y < 2 &&
			cell_coords.z < 2;
	}

	uint8_t coordToIndex(const glm::vec3& coords) const
	{
		return coords.z * 4 + coords.y * 2 + coords.x;
	}

	bool hasChild(const uint8_t child_mask, const uint8_t index) const
	{
		return (child_mask >> index) & 1U;
	}

	bool isLeaf(uint8_t leaf_mask, uint8_t index) const
	{
		return (leaf_mask >> index) & 1U;
	}

	void rec_castRay(Ray& ray, const glm::vec3& position, uint32_t cell_size, const uint32_t node_index, const uint32_t max_iter) const 
	{
		glm::vec3 cell_pos_i = glm::ivec3(position.x / cell_size, position.y / cell_size, position.z / cell_size);
		clamp(cell_pos_i.x, 0.0f, 1.0f);
		clamp(cell_pos_i.y, 0.0f, 1.0f);
		clamp(cell_pos_i.z, 0.0f, 1.0f);
		glm::vec3 t_max = ((cell_pos_i + ray.dir) * float(cell_size) - position) / ray.direction;

		const glm::vec3 t = float(cell_size) * ray.t;

		const LNode current_node = data[node_index];

		float t_max_min = 0.0f;
		const float t_total = ray.t_total;
		while (checkCell(cell_pos_i) && ray.point.complexity < max_iter) {
			const uint8_t sub_index = coordToIndex(cell_pos_i);
			// Increase pixel complexity
			++ray.point.complexity;
			// We enter the sub node
			if (hasChild(current_node.child_mask, sub_index)) {
				if (isLeaf(current_node.leaf_mask, sub_index)) {
					fillHitResult(ray, t_total + t_max_min);
					return;
				}
				else {
					const uint32_t sub_size = cell_size >> 1U;
					const glm::vec3 sub_position = (position + t_max_min * ray.direction) - cell_pos_i * float(cell_size);
					ray.t_total = t_total + t_max_min;
					rec_castRay(ray, sub_position, sub_size, node_index + current_node.child_offset + sub_index, max_iter);
					if (ray.point.cell) {
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


	std::vector<LNode> data;
	uint8_t max_level;
	Cell* cell;
};
