#pragma once

struct Cell
{
	enum Type {
		Empty,
		Solid,
		Mirror,
		Grass,
		Water
	};

	Cell()
		: type(Type::Empty)
	{}

	Type type;
};
