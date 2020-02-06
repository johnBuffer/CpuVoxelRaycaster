#pragma once

#include "ray.hpp"
#include "lsvo_utils.hpp"
#include "volumetric.hpp"
#include <bitset>


template<uint8_t MAX_DEPTH>
struct LSVO : public Volumetric
{
	LSVO(const SVO& svo)
	{
		importFromSVO(svo);
	}

	void importFromSVO(const SVO& svo)
	{
		data = compileSVO(svo);
		cell = new Cell();
		cell->type = Cell::Type::Solid;
		cell->texture = Cell::Texture::Grass;
	}

	void setCell(Cell::Type type, Cell::Texture texture, uint32_t x, uint32_t y, uint32_t z) {}

	glm::vec3 getT(const glm::vec3& planes_pos, const glm::vec3& inv_direction, const glm::vec3& offset) const
	{
		return planes_pos * inv_direction - offset;
	}

	vec3bool getChildPos(const glm::vec3& t_center, float t_min) const
	{
		return vec3bool(t_center.x > t_min, t_center.y > t_min, t_center.z > t_min);
	}

	HitPoint castRay(const glm::vec3& position, const glm::vec3& direction, const uint32_t max_iter) const
	{
		HitPoint result;
		// Check octant mask and modify ray accordingly
		uint8_t octant_mask = 0u;
		glm::vec3 r_dir = glm::normalize(direction);
		if (r_dir.x > 0.0f) r_dir.x = -r_dir.x, octant_mask ^= 1u;
		if (r_dir.y > 0.0f) r_dir.y = -r_dir.y, octant_mask ^= 2u;
		if (r_dir.z > 0.0f) r_dir.z = -r_dir.z, octant_mask ^= 4u;
		const glm::vec3 r_inv = 1.0f / r_dir;
		const glm::vec3 r_off = position * r_inv;
		// Initialize stack
		OctreeStack stack[MAX_DEPTH];
		int8_t scale = MAX_DEPTH - 1;
		uint32_t size = std::pow(2U, scale + 1U);
		float size_f = 0.5f;
		// Initialize t_span
		const glm::vec3 t0 = getT(glm::vec3(1.0f), r_inv, r_off);
		const glm::vec3 t1 = getT(glm::vec3(0.0f), r_inv, r_off);

		float t_min = (std::max(t0.x, std::max(t0.y, t0.z)));
		float t_max = (std::min(t1.x, std::min(t1.y, t1.z)));
		float h = t_max;

		// Initialize t_center
		const glm::vec3 t_center = getT(glm::vec3(0.5f), r_inv, r_off);
		// Initialize child position
		const LNode* parent = &(data[0]);
		const LNode* child_ptr = nullptr;
		uint32_t child_id = 0u;
		glm::vec3 pos(0.0f);
		//child_pos.data ^= octant_mask;
		if (t_center.x > t_min) { child_id ^= 1u, pos.x = 0.5f; }
		if (t_center.y > t_min) { child_id ^= 2u, pos.y = 0.5f; }
		if (t_center.z > t_min) { child_id ^= 4u, pos.z = 0.5f; }
		// Explore octree
		while (scale < MAX_DEPTH) {
			if (!child_ptr) {
				child_ptr = parent;
			}
			// Compute new T span
			const glm::vec3 t_corner = getT(pos, r_inv, r_off);
			const float tc_max = std::min(t_corner.x, std::min(t_corner.y, t_corner.z));
			// Check if child exists here
			const uint8_t child_mask = parent->child_mask >> child_id;
			if ((child_mask & 1u) && t_min <= t_max) {
				const float tv_max = std::min(tc_max, t_max);
				const float half = size_f * 0.5f;

				const glm::vec3 t_half = half * r_inv + t_corner;

				if (t_min <= tv_max) {

				}
			}
		}
		
		return result;
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
		
	}

	uint8_t coordToIndex(const glm::vec3& coords) const
	{
		return coords.z * 4 + coords.y * 2 + coords.x;
	}

	bool hasChild(const LNode node, const uint8_t index) const
	{
		return (node.child_mask >> index) & 1U;
	}

	bool isLeaf(const LNode node, uint8_t index) const
	{
		return (node.leaf_mask >> index) & 1U;
	}

	std::vector<LNode> data;
	Cell* cell;
};
