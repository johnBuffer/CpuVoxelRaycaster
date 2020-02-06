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
		float scale_f = 0.5f;
		// Initialize t_span
		const glm::vec3 t0 = getT(glm::vec3(2.0f), r_inv, r_off);
		const glm::vec3 t1 = getT(glm::vec3(1.0f), r_inv, r_off);

		float t_min = (std::max(t0.x, std::max(t0.y, t0.z)));
		float t_max = (std::min(t1.x, std::min(t1.y, t1.z)));
		float h = t_max;

		// Initialize t_center
		const glm::vec3 t_center = getT(glm::vec3(1.5f), r_inv, r_off);
		// Initialize child position
		const LNode* raw_data = &(data[0]);
		uint32_t parent = 0u;
		uint32_t child = 0u;
		uint32_t child_id = 0u;
		glm::vec3 pos(0.0f);
		//child_pos.data ^= octant_mask;
		if (t_center.x > t_min) { child_id ^= 1u, pos.x = 1.5f; }
		if (t_center.y > t_min) { child_id ^= 2u, pos.y = 1.5f; }
		if (t_center.z > t_min) { child_id ^= 4u, pos.z = 1.5f; }
		// std::cout << "START at " << (child_id ^ octant_mask) << std::endl;
		// Explore octree
		while (scale < MAX_DEPTH && result.complexity < max_iter) {
			++result.complexity;
			if (!child) {
				child = parent;
			}
			//std::string indent = "";
			//for (uint32_t i(0); i<MAX_DEPTH - scale; ++i) { indent += "  "; }
			// Compute new T span
			const glm::vec3 t_corner = getT(pos, r_inv, r_off);
			const float tc_max = std::min(t_corner.x, std::min(t_corner.y, t_corner.z));
			// Check if child exists here
			const LNode& parent_ref = raw_data[parent];
			const uint8_t child_mask = parent_ref.child_mask >> (child_id ^ octant_mask);
			if ((child_mask & 1u) && t_min <= t_max) {
				const float tv_max = std::min(tc_max, t_max);
				const float half = scale_f * 0.5f;
				const glm::vec3 t_half = half * r_inv + t_corner;
				if (t_min <= tv_max) {
					const uint8_t leaf_mask = parent_ref.leaf_mask >> (child_id ^ octant_mask);
					// We hit a leaf
					if (leaf_mask & 1u) {
						//std::cout << indent << int32_t(scale) << " HIT at " << toString(position + t_min * direction) << std::endl;
						break;
					}
					// Eventually add parent to the stack
					if (tc_max < h) {
						stack[scale].parent_index = parent;
						stack[scale].t_max = t_max;
						stack[scale].valid = true;
					}
					else {
						stack[scale].valid = false;
					}
					h = tc_max;
					// Update current voxel
					parent += parent_ref.child_offset + (child_id ^ octant_mask);
					--scale;
					child_id = 0u;
					scale_f = half;
					if (t_half.x > t_min) { child_id ^= 1u, pos.x += scale_f; }
					if (t_half.y > t_min) { child_id ^= 2u, pos.y += scale_f; }
					if (t_half.z > t_min) { child_id ^= 4u, pos.z += scale_f; }
					//std::cout << indent << int32_t(scale) << " PUSH, new child_id " << (child_id ^ octant_mask) << std::endl;
					t_max = tv_max;
					child = 0u;
					continue;
				}
			} // End of depth exploration

			uint32_t step_mask = 0u;
			if (t_corner.x <= tc_max) { step_mask ^= 1u, pos.x -= scale_f; }
			if (t_corner.y <= tc_max) { step_mask ^= 2u, pos.y -= scale_f; }
			if (t_corner.z <= tc_max) { step_mask ^= 4u, pos.z -= scale_f; }

			t_min = tc_max;
			child_id ^= step_mask;
			//std::cout << indent << int32_t(scale) << " ADVANCE, new child_id " << (child_id ^ octant_mask) << std::endl;

			if (child_id & step_mask) {
				while (!stack[++scale].valid) {}
				OctreeStack entry = stack[scale];
				parent = entry.parent_index;
				t_max = entry.t_max;

				uint32_t shx = floatAsInt(pos.x) >> scale;
				uint32_t shy = floatAsInt(pos.y) >> scale;
				uint32_t shz = floatAsInt(pos.z) >> scale;
				pos.x = intAsFloat(shx << scale);
				pos.y = intAsFloat(shy << scale);
				pos.z = intAsFloat(shz << scale);
				child_id = (shx & 1u) | ((shy & 1u) << 1u) | ((shz & 1u) << 2u);

				h = 0.0f;
				child = 0;

				//std::cout << indent << "POP, new scale " << int32_t(scale)  << std::endl;
			}
		}
		
		result.distance = t_min;
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

	std::vector<LNode> data;
	Cell* cell;
};
