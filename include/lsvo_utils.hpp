#pragma once

#include "svo.hpp"

struct LNode
{
	LNode()
		: child_mask(0U)
		, leaf_mask(0U)
		, child_offset(0U)
		, color(1u)
	{}

	uint8_t  color;
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


void compileSVO_rec(const Node* node, std::vector<LNode>& data, const uint32_t node_index, uint32_t& max_offset);


template<uint8_t N>
std::vector<LNode> compileSVO(const SVO<N>& svo)
{
	std::vector<LNode> data;
	data.push_back(LNode());

	uint32_t max_offset = 0U;
	compileSVO_rec(svo.m_root, data, 0, max_offset);

	return data;
}