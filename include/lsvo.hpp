#pragma once

#include "svo.hpp"


struct LNode
{
	uint8_t  child_mask;
	uint32_t child_offset;
};


std::vector<LNode> compileSVO(const SVO& svo)
{

}


void compileSVO_rec(const Node* node, std::vector<LNode>& vector)
{

}