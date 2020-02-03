#pragma once

#include "svo.hpp"

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


struct vec3bool
{
	vec3bool() : data(0U) {}
	vec3bool(uint8_t x_, uint8_t y_, uint8_t z_)
		: data(x_ | (y_ << 1U) | (z_ << 2U))
	{}

	bool x() const { return data & 0b001; }
	bool y() const { return data & 0b010; }
	bool z() const { return data & 0b100; }
	uint8_t data;
};


struct OctreeStack
{
	uint32_t parent_index;
	float t_max;
};


struct LRay
{
	LRay(const glm::vec3& start_, const glm::vec3& direction_)
		: start(start_)
		, direction(glm::normalize(direction_))
		, inv_direction(1.0f / direction_)
		, offset(-start_ / direction_)
	{}

	const glm::vec3 getT(const glm::vec3& planes) const
	{
		return planes * inv_direction + offset;
	}

	const glm::vec3 start;
	const glm::vec3 direction;
	const glm::vec3 inv_direction;
	const glm::vec3 offset;
};


void compileSVO_rec(const Node* node, std::vector<LNode>& data, const uint32_t node_index, uint32_t& max_offset);

std::vector<LNode> compileSVO(const SVO& svo);
