#pragma once

#include "lsvo.hpp"
#include <iostream>


bool hasChild(const uint8_t child_mask, const uint8_t index)
{
	return (child_mask >> index) & 1U;
}

bool isLeaf(uint8_t leaf_mask, uint8_t index)
{
	return (leaf_mask >> index) & 1U;
}


template<uint32_t N>
void print(const LSVO<N>& svo)
{
	print_rec(svo, 0, 0, "");
}


template<uint32_t N>
void print_rec(const LSVO<N>& svo, uint32_t node_index, uint32_t local_id, const std::string& level_indent)
{
	const std::string indent = "  ";
	LNode current_node = svo.data[node_index];
	std::cout << level_indent << "Local ID " << local_id << std::endl;

	for (uint32_t i(0); i < 8U; ++i) {
		const uint32_t child_index = node_index + current_node.child_offset + i;
		if (hasChild(current_node.child_mask, i)) {
			if (isLeaf(current_node.leaf_mask, i)) {
				std::cout << level_indent + indent << "Local ID " << i << " LEAF (" << i % 2 << ", " << (i / 2U) % 2 << ", " << i / 4U << ")" << std::endl;
			}
			else {
				print_rec(svo, child_index, i, level_indent + indent);
			}
		}
	}
}
