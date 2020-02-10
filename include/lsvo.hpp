#pragma once

#include "ray.hpp"
#include "lsvo_utils.hpp"
#include "volumetric.hpp"
#include <bitset>


template<uint8_t MAX_DEPTH>
struct LSVO : public Volumetric
{
	LSVO(const SVO<MAX_DEPTH>& svo)
	{
		importFromSVO(svo);
		raw_data = &(data[0]);
	}

	void importFromSVO(const SVO<MAX_DEPTH>& svo)
	{
		data = compileSVO(svo);
		cell = new Cell();
		cell->type = Cell::Type::Solid;
		cell->texture = Cell::Texture::Grass;
	}

	void setCell(Cell::Type type, Cell::Texture texture, uint32_t x, uint32_t y, uint32_t z) {}

	inline glm::vec3 getT(const glm::vec3& planes_pos, const glm::vec3& inv_direction, const glm::vec3& offset) const
	{
		return planes_pos * inv_direction - offset;
	}

	HitPoint castRay(const glm::vec3& position, glm::vec3 d, const float ray_size_coef = 0.0f, const float ray_size_bias = 0.0f) const override
	{
		HitPoint result;
		// Const values
		constexpr uint8_t SVO_MAX_DEPTH = 23u;
		constexpr uint8_t DEPTH_OFFSET = SVO_MAX_DEPTH - MAX_DEPTH;
		constexpr float SVO_SIZE = 1 << MAX_DEPTH;
		constexpr float EPS = 1.0f / float(1 << SVO_MAX_DEPTH);
		// Initialize stack
		OctreeStack stack[MAX_DEPTH];
		// Check octant mask and modify ray accordingly
		if (std::abs(d.x) < EPS) { d.x = copysign(EPS, d.x); }
		if (std::abs(d.y) < EPS) { d.y = copysign(EPS, d.y); }
		if (std::abs(d.z) < EPS) { d.z = copysign(EPS, d.z); }
		const glm::vec3 t_coef = -1.0f / glm::abs(d);
		glm::vec3 t_offset = position * t_coef;
		uint8_t mirror_mask = 7u;
		if (d.x > 0.0f) { mirror_mask ^= 1u, t_offset.x = 3.0f * t_coef.x - t_offset.x; }
		if (d.y > 0.0f) { mirror_mask ^= 2u, t_offset.y = 3.0f * t_coef.y - t_offset.y; }
		if (d.z > 0.0f) { mirror_mask ^= 4u, t_offset.z = 3.0f * t_coef.z - t_offset.z; }
		// Initialize t_span
		float t_min = std::max(2.0f * t_coef.x - t_offset.x, std::max(2.0f * t_coef.y - t_offset.y, 2.0f * t_coef.z - t_offset.z));
		float t_max = std::min(t_coef.x - t_offset.x, std::min(t_coef.y - t_offset.y, t_coef.z - t_offset.z));
		float h = t_max;
		t_min = std::max(0.0f, t_min);
		t_max = std::min(1.0f, t_max);
		// Init current voxel
		uint32_t parent_id = 0u;
		uint8_t child_offset = 0u;
		int8_t scale = SVO_MAX_DEPTH - 1u;
		glm::vec3 pos(1.0f);
		float scale_f = 0.5f;
		// Initialize child position
		if (1.5f * t_coef.x - t_offset.x > t_min) { child_offset ^= 1u, pos.x = 1.5f; }
		if (1.5f * t_coef.y - t_offset.y > t_min) { child_offset ^= 2u, pos.y = 1.5f; }
		if (1.5f * t_coef.z - t_offset.z > t_min) { child_offset ^= 4u, pos.z = 1.5f; }
		uint8_t normal = 0u;
		uint16_t child_infos = 0u;
		// Explore octree
		while (scale < SVO_MAX_DEPTH && scale > MAX_DEPTH) {
			++result.complexity;
			const LNode& parent_ref = raw_data[parent_id];
			// Compute new T span
			const glm::vec3 t_corner(pos.x * t_coef.x - t_offset.x, pos.y * t_coef.y - t_offset.y, pos.z * t_coef.z - t_offset.z);
			const float tc_max = std::min(t_corner.x, std::min(t_corner.y, t_corner.z));
			// Check if child exists here
			const uint8_t child_shift = child_offset ^ mirror_mask;
			const uint8_t child_mask = parent_ref.child_mask >> child_shift;
			if ((child_mask & 1u) && t_min <= t_max) {
				if (tc_max * ray_size_coef + ray_size_bias >= scale_f) {
					result.cell = cell;
					break;
				}
				const float tv_max = std::min(t_max, tc_max);
				const float half = scale_f * 0.5f;
				const glm::vec3 t_half = half * t_coef + t_corner;
				if (t_min <= tv_max) {
					const uint8_t leaf_mask = parent_ref.leaf_mask >> child_shift;
					// We hit a leaf
					if (leaf_mask & 1u) {
						result.cell = cell;
						break;
					}
					// Eventually add parent to the stack
					if (tc_max < h) {
						stack[scale - DEPTH_OFFSET].parent_index = parent_id;
						stack[scale - DEPTH_OFFSET].t_max = t_max;
					}
					h = tc_max;
					// Update current voxel
					parent_id += parent_ref.child_offset + child_shift;
					child_offset = 0u;
					--scale;
					scale_f = half;
					if (t_half.x > t_min) { child_offset ^= 1u, pos.x += scale_f; }
					if (t_half.y > t_min) { child_offset ^= 2u, pos.y += scale_f; }
					if (t_half.z > t_min) { child_offset ^= 4u, pos.z += scale_f; }
					t_max = tv_max;
					continue;
				}
			} // End of depth exploration

			uint32_t step_mask = 0u;
			if (t_corner.x <= tc_max) { step_mask ^= 1u, pos.x -= scale_f; }
			if (t_corner.y <= tc_max) { step_mask ^= 2u, pos.y -= scale_f; }
			if (t_corner.z <= tc_max) { step_mask ^= 4u, pos.z -= scale_f; }

			t_min = tc_max;
			child_offset ^= step_mask;
			normal = step_mask;

			if (child_offset & step_mask) {
				uint32_t differing_bits = 0u;
				const int32_t ipos_x = floatAsInt(pos.x);
				const int32_t ipos_y = floatAsInt(pos.y);
				const int32_t ipos_z = floatAsInt(pos.z);
				if (step_mask & 1u) differing_bits |= (ipos_x ^ floatAsInt(pos.x + scale_f));
				if (step_mask & 2u) differing_bits |= (ipos_y ^ floatAsInt(pos.y + scale_f));
				if (step_mask & 4u) differing_bits |= (ipos_z ^ floatAsInt(pos.z + scale_f));
				scale = (floatAsInt((float)differing_bits) >> SVO_MAX_DEPTH) - 127u;
				scale_f = intAsFloat((scale - SVO_MAX_DEPTH + 127u) << SVO_MAX_DEPTH);
				const OctreeStack entry = stack[scale - DEPTH_OFFSET];
				parent_id = entry.parent_index;
				t_max = entry.t_max;
				const uint32_t shx = ipos_x >> scale;
				const uint32_t shy = ipos_y >> scale;
				const uint32_t shz = ipos_z >> scale;
				pos.x = intAsFloat(shx << scale);
				pos.y = intAsFloat(shy << scale);
				pos.z = intAsFloat(shz << scale);
				child_offset = (shx & 1u) | ((shy & 1u) << 1u) | ((shz & 1u) << 2u);
				h = 0.0f;
			}
		}
		
		if (result.cell) {
			result.normal = -glm::sign(d) * glm::vec3(float(normal & 1u), float(normal & 2u), float(normal & 4u));

			if ((mirror_mask & 1) == 0) pos.x = 3.0f - scale_f - pos.x;
			if ((mirror_mask & 2) == 0) pos.y = 3.0f - scale_f - pos.y;
			if ((mirror_mask & 4) == 0) pos.z = 3.0f - scale_f - pos.z;

			result.distance = t_min;
			result.position.x = std::min(std::max(position.x + t_min * d.x, pos.x + EPS), pos.x + scale_f - EPS);
			result.position.y = std::min(std::max(position.y + t_min * d.y, pos.y + EPS), pos.y + scale_f - EPS);
			result.position.z = std::min(std::max(position.z + t_min * d.z, pos.z + EPS), pos.z + scale_f - EPS);

			if (result.normal.x) {
				result.voxel_coord = glm::vec2(frac(result.position.z * SVO_SIZE), frac(result.position.y * SVO_SIZE));
			}
			else if (result.normal.y) {
				result.voxel_coord = glm::vec2(frac(result.position.x * SVO_SIZE), frac(result.position.z * SVO_SIZE));
			}
			else if (result.normal.z) {
				result.voxel_coord = glm::vec2(frac(result.position.x * SVO_SIZE), frac(result.position.y * SVO_SIZE));
			}
		}

		return result;
	}

	LNode* getAtRayHit(const glm::vec3& position, glm::vec3 d)
	{
		HitPoint result;
		// Const values
		constexpr uint8_t SVO_MAX_DEPTH = 23u;
		constexpr uint8_t DEPTH_OFFSET = SVO_MAX_DEPTH - MAX_DEPTH;
		constexpr float SVO_SIZE = 1 << MAX_DEPTH;
		constexpr float epsilon = 1.0f / float(1 << SVO_MAX_DEPTH);
		const LNode* raw_data = &(data[0]);
		// Initialize stack
		OctreeStack stack[MAX_DEPTH];
		// Check octant mask and modify ray accordingly
		if (std::abs(d.x) < epsilon) { d.x = copysign(epsilon, d.x); }
		if (std::abs(d.y) < epsilon) { d.y = copysign(epsilon, d.y); }
		if (std::abs(d.z) < epsilon) { d.z = copysign(epsilon, d.z); }
		const glm::vec3 t_coef = -1.0f / glm::abs(d);
		glm::vec3 t_offset = position * t_coef;
		uint8_t mirror_mask = 7u;
		if (d.x > 0.0f) { mirror_mask ^= 1u, t_offset.x = 3.0f * t_coef.x - t_offset.x; }
		if (d.y > 0.0f) { mirror_mask ^= 2u, t_offset.y = 3.0f * t_coef.y - t_offset.y; }
		if (d.z > 0.0f) { mirror_mask ^= 4u, t_offset.z = 3.0f * t_coef.z - t_offset.z; }
		// Initialize t_span
		const glm::vec3 t0 = getT(glm::vec3(2.0f), t_coef, t_offset);
		const glm::vec3 t1 = getT(glm::vec3(1.0f), t_coef, t_offset);
		float t_min = std::max(t0.x, std::max(t0.y, t0.z));
		float t_max = std::min(t1.x, std::min(t1.y, t1.z));
		float h = t_max;
		t_min = std::max(0.0f, t_min);
		t_max = std::min(1.0f, t_max);
		// Init current voxel
		uint32_t parent_id = 0u;
		uint8_t child_offset = 0u;
		int8_t scale = SVO_MAX_DEPTH - 1U;
		glm::vec3 pos(1.0f);
		float scale_f = 0.5f;
		// Initialize child position
		const glm::vec3 t_center = getT(glm::vec3(1.5f), t_coef, t_offset);
		if (t_center.x > t_min) { child_offset ^= 1u, pos.x = 1.5f; }
		if (t_center.y > t_min) { child_offset ^= 2u, pos.y = 1.5f; }
		if (t_center.z > t_min) { child_offset ^= 4u, pos.z = 1.5f; }
		uint8_t normal = 0u;
		// Explore octree
		while (scale < SVO_MAX_DEPTH && scale > MAX_DEPTH) {
			++result.complexity;
			const LNode& parent_ref = raw_data[parent_id];
			// Compute new T span
			const glm::vec3 t_corner = getT(pos, t_coef, t_offset);
			const float tc_max = std::min(t_corner.x, std::min(t_corner.y, t_corner.z));
			// Check if child exists here
			const uint8_t child_shift = child_offset ^ mirror_mask;
			const uint8_t child_mask = parent_ref.child_mask >> child_shift;
			if ((child_mask & 1u) && t_min <= t_max) {
				const float tv_max = std::min(t_max, tc_max);
				const float half = scale_f * 0.5f;
				const glm::vec3 t_half = half * t_coef + t_corner;
				if (t_min <= tv_max) {
					const uint8_t leaf_mask = parent_ref.leaf_mask >> child_shift;
					// We hit a leaf
					if (leaf_mask & 1u) {
						return &parent_ref;
					}
					// Eventually add parent to the stack
					if (tc_max < h) {
						stack[scale - DEPTH_OFFSET].parent_index = parent_id;
						stack[scale - DEPTH_OFFSET].t_max = t_max;
					}
					h = tc_max;
					// Update current voxel
					parent_id += parent_ref.child_offset + child_shift;
					child_offset = 0u;
					--scale;
					scale_f = half;
					if (t_half.x > t_min) { child_offset ^= 1u, pos.x += scale_f; }
					if (t_half.y > t_min) { child_offset ^= 2u, pos.y += scale_f; }
					if (t_half.z > t_min) { child_offset ^= 4u, pos.z += scale_f; }
					t_max = tv_max;
					continue;
				}
			} // End of depth exploration

			uint32_t step_mask = 0u;
			if (t_corner.x <= tc_max) { step_mask ^= 1u, pos.x -= scale_f; }
			if (t_corner.y <= tc_max) { step_mask ^= 2u, pos.y -= scale_f; }
			if (t_corner.z <= tc_max) { step_mask ^= 4u, pos.z -= scale_f; }

			t_min = tc_max;
			child_offset ^= step_mask;
			normal = step_mask;

			if (child_offset & step_mask) {
				uint32_t differing_bits = 0u;
				if (step_mask & 1u) differing_bits |= (floatAsInt(pos.x) ^ floatAsInt(pos.x + scale_f));
				if (step_mask & 2u) differing_bits |= (floatAsInt(pos.y) ^ floatAsInt(pos.y + scale_f));
				if (step_mask & 4u) differing_bits |= (floatAsInt(pos.z) ^ floatAsInt(pos.z + scale_f));
				scale = (floatAsInt((float)differing_bits) >> SVO_MAX_DEPTH) - 127u;
				scale_f = intAsFloat((scale - SVO_MAX_DEPTH + 127u) << SVO_MAX_DEPTH);
				OctreeStack entry = stack[scale - DEPTH_OFFSET];
				parent_id = entry.parent_index;
				t_max = entry.t_max;
				const uint32_t shx = floatAsInt(pos.x) >> scale;
				const uint32_t shy = floatAsInt(pos.y) >> scale;
				const uint32_t shz = floatAsInt(pos.z) >> scale;
				pos.x = intAsFloat(shx << scale);
				pos.y = intAsFloat(shy << scale);
				pos.z = intAsFloat(shz << scale);
				child_offset = (shx & 1u) | ((shy & 1u) << 1u) | ((shz & 1u) << 2u);
				h = 0.0f;
			}
		}

		return nullptr;
	}

	std::vector<LNode> data;
	const LNode* raw_data;
	Cell* cell;
};
