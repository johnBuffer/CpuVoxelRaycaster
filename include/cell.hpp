#pragma once

struct Material
{
	sf::Color color;
	float reflectivity;
};


struct Cell
{
	enum Type {
		Empty,
		Solid
	};

	Cell()
		: type(Type::Empty)
		, material(nullptr)
	{}

	Type type;
	const Material* material;
};
