#pragma once

#include "svo.hpp"


struct LNode
{
	uint8_t  child_mask;
	uint32_t child_offset;
};


void compileSVO_rec(const Node* node, std::vector<LNode>& data, const uint32_t node_index)
{
	if (node) {
		const uint32_t child_pos = data.size();
		const uint32_t offset = child_pos - node_index;
		if (child_pos) {
			data[node_index].child_offset = offset;
		}

		for (uint8_t i(8U); i--;) {
			data.push_back(LNode());
		}

		for (uint8_t x(0U); x < 2; ++x) {
			for (uint8_t y(0U); y < 2; ++y) {
				for (uint8_t z(0U); z < 2; ++z) {
					const Node* sub_node = node->sub[x][y][z];
					LNode new_node;
					if (sub_node) {
						const uint8_t sub_index = z * 4 + y * 2 + x;
						if (child_pos) {
							data[node_index].child_mask |= (1U) << sub_index;
						}
						compileSVO_rec(sub_node, data, child_pos + sub_index);
					}
				}
			}
		}
	}
}


std::vector<LNode> compileSVO(const SVO& svo)
{
	std::vector<LNode> data;

	compileSVO_rec(svo.m_root, data, 0);

	return data;
}
