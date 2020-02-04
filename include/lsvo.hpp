#pragma once

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

	glm::vec3 getT(const LRay& ray) const
	{
		return -0.5f * (ray.sign * ray.inv_direction);
	}

	vec3bool getChildPos(const glm::vec3& t_center, const glm::vec3& t_ray) const
	{
		return vec3bool(t_ray.x < t_center.x, t_ray.y < t_center.y, t_ray.z < t_center.z);
	}

	HitPoint castRay(const glm::vec3& position, const glm::vec3& direction, const uint32_t max_iter) const
	{
		LRay ray(position, direction);
		HitPoint result;
		// Initialize stack
		OctreeStack stack[MAX_DEPTH];
		int8_t scale = MAX_DEPTH - 1;
		uint32_t size = std::pow(2U, scale + 1U);
		// Initialize t_span
		const glm::vec3 t0 = ray.getT(glm::vec3(0.0f));
		const glm::vec3 t1 = ray.getT(glm::vec3(static_cast<float>(size)));

		glm::vec2 t_span;
		t_span.x = (std::max(t0.x, std::max(t0.y, t0.z)));
		t_span.y = (std::min(t1.x, std::min(t1.y, t1.z)));
		// Initialize t_center
		const glm::vec3 t_center = getT(ray);
		// Initialize child position
		vec3bool child_pos = getChildPos(float(size) * t_center, t0);
		// Iterating through the Octree
		
		std::cout << std::bitset<8>(child_pos.data) << std::endl;

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
