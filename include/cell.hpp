#pragma once

struct Cell
{
	enum Type {
		Empty,
		Solid,
		Mirror
	};

	Cell()
		: type(Type::Empty)
	{}

	Type type;
};
