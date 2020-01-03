#pragma once

struct Cell
{
	enum Type {
		Empty,
		Solid,
		Mirror
	};

	enum Texture {
		None,
		Grass
	};

	Cell()
		: type(Type::Empty)
	{}

	Type type;
	Texture texture;
};
