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
		Grass,
		Red,
		White
	};

	Cell()
		: type(Type::Empty)
	{}

	Type type;
	Texture texture;
};
