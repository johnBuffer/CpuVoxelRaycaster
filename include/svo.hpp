#pragma once

#include "volumetric.hpp"
#include "utils.hpp"

template<uint32_t SUB>
struct Node
{
	Node()
	{
		for (int x(0); x < SUB; ++x) {
			for (int y(0); y < SUB; ++y) {
				for (int z(0); z < SUB; ++z) {
					sub[x][y][z] = nullptr;
				}
			}
		}

		leaf = false;
	}

	Node* sub[SUB][SUB][SUB];
	bool leaf;
	Cell cell;
};


struct Ray
{
	Ray() = default;
	
	Ray(const glm::vec3& start_, const glm::vec3& direction_)
		: start(start_)
		, direction(direction_)
		//, inv_direction(1.0f / direction_)
		, t(glm::abs(1.0f / direction_))
		, step(copysign(1.0f, direction_.x), copysign(1.0f, direction_.y), copysign(1.0f, direction_.z))
		, dir(direction_.x > 0.0f, direction_.y > 0.0f, direction_.z > 0.0f)
		, point()
		, t_total(0.0f)
		, hit_side(0U)
	{
	}

	const glm::vec3 start;
	const glm::vec3 direction;
	//const glm::vec3 inv_direction;
	const glm::vec3 t;
	const glm::vec3 step;
	const glm::vec3 dir;
	uint8_t hit_side;
	float t_total;

	HitPoint point;
};


template<uint32_t SUB>
struct StackContext
{
	StackContext()
		: position(0.0f)
		, cell_position(0.0f)
		, t(0.0f)
		, t_min(0.0f)
		, t_max_min(0.0f)
		, t_sum(0.0f)
		, cell_size(1U)
		, check_cell(true)
		, node(nullptr)
	{

	}

	glm::vec3 position;
	glm::vec3 cell_position;
	glm::vec3 t;
	glm::vec3 t_min;

	float t_max_min;
	float t_sum;

	uint32_t cell_size;

	bool check_cell;

	const Node<SUB>* node;
};

template<uint32_t SUB>
struct Stack
{
	Stack()
		: current_level(0)
	{}

	int32_t current_level;
	StackContext<SUB> data[10];
};


template<uint32_t SUB>
class SVO
{
public:
	SVO()
	{
		m_root = new Node<SUB>();
	}

	HitPoint castRay(const glm::vec3& position, const glm::vec3& direction) const
	{
		Ray ray(position, direction);

		const uint32_t max_cell_size = uint32_t(std::pow(SUB, m_max_level - 1));
		//stack_castRay(ray, position, max_cell_size, m_root);
		rec_castRay(ray, position, max_cell_size, m_root);

		return ray.point;
	}

	void setCell(Cell::Type type, Cell::Texture texture, uint32_t x, uint32_t y, uint32_t z)
	{
		const uint32_t max_size = uint32_t(std::pow(SUB, m_max_level));
		rec_setCell(type, texture, x, y, z, m_root, max_size);
	}

	inline static bool checkCell(const glm::vec3& cell_coords)
	{
		return cell_coords.x >= 0 &&
			   cell_coords.y >= 0 &&
			   cell_coords.z >= 0 &&
			   cell_coords.x < SUB &&
			   cell_coords.y < SUB &&
			   cell_coords.z < SUB;
	}

private:
	Node<SUB>* m_root;
	const uint32_t m_max_level = 10U;

	void rec_setCell(Cell::Type type, Cell::Texture texture, uint32_t x, uint32_t y, uint32_t z, Node<SUB>* node, uint32_t size)
	{
		if (!node) {
			return;
		}

		if (size == 1) {
			node->cell.type = type;
			node->cell.texture = texture;
			node->leaf = true;
			return;
		}

		const uint32_t sub_size = size / SUB;
		const uint32_t cell_x = x / sub_size;
		const uint32_t cell_y = y / sub_size;
		const uint32_t cell_z = z / sub_size;

		if (!node->sub[cell_x][cell_y][cell_z]) {
			node->sub[cell_x][cell_y][cell_z] = new Node<SUB>();
		}

		rec_setCell(type, texture, x - cell_x * sub_size, y - cell_y * sub_size, z - cell_z * sub_size, node->sub[cell_x][cell_y][cell_z], sub_size);
	}
	
	void initializeStack(Stack<SUB>& stack, Ray& ray, const glm::vec3& position, const uint32_t cell_size, const Node<SUB>* node) const
	{
		stack.current_level = 0;
		StackContext<SUB>& context = stack.data[0];

		context.cell_size = cell_size;
		context.position = position;
		context.t_sum = 0.0f;
		context.node = node;
		context.cell_position = glm::ivec3(position.x / cell_size, position.y / cell_size, position.z / cell_size);
		context.t_min = glm::vec3((context.cell_position + ray.dir) * float(cell_size) - context.position) / ray.direction;
		context.t = float(cell_size) * ray.t;
		context.check_cell = true;
		context.t_max_min = 0.0f;
	}
	
	void addStackContext(Stack<SUB>& stack, Ray& ray, const Node<SUB>* node) const
	{
		const int32_t current_level = stack.current_level;
		const int32_t next_level = current_level + 1;
		const StackContext<SUB>& current_context = stack.data[current_level];
		StackContext<SUB>& next_context = stack.data[next_level];

		const uint32_t sub_cell_size = current_context.cell_size / SUB;
		next_context.cell_size = sub_cell_size;
		const glm::vec3 position = (current_context.position + current_context.t_max_min * ray.direction) - current_context.cell_position * float(current_context.cell_size);
		next_context.position = position;
		next_context.t_sum = current_context.t_sum + current_context.t_max_min;
		next_context.node = node;
		next_context.cell_position = glm::clamp(glm::ivec3(position.x / sub_cell_size, position.y / sub_cell_size, position.z / sub_cell_size), glm::ivec3(0), glm::ivec3(SUB-1));;

		next_context.t_min = glm::vec3((next_context.cell_position + ray.dir) * float(sub_cell_size) - next_context.position) / ray.direction;
		next_context.t = 0.5f * current_context.t;
		next_context.check_cell = true;
		next_context.t_max_min = 0.0f;

		stack.current_level = next_level;
	}
	
	void stack_castRay(Ray& ray, const glm::vec3& position, uint32_t cell_size, const Node<SUB>* node) const
	{
		Stack<SUB> stack;

		// Initialize stack
		initializeStack(stack, ray, position, cell_size, node);

		while (stack.current_level > -1) {
			StackContext<SUB>& context = stack.data[stack.current_level];
			glm::vec3& cell_position = context.cell_position;

			while (checkCell(cell_position)) {
				// Increase pixel complexity
				++ray.point.complexity;

				if (context.check_cell) {
					context.check_cell = false;
					// We enter the sub node
					const Node<SUB>* sub_node = context.node->sub[uint32_t(cell_position.x)][uint32_t(cell_position.y)][uint32_t(cell_position.z)];
					if (sub_node) {
						if (sub_node->leaf) {
							HitPoint& point = ray.point;
							point.distance = context.t_sum + context.t_max_min;
							const Cell& cell = sub_node->cell;
							const glm::vec3 hit = ray.start + point.distance * ray.direction;

							point.cell = &cell;
							point.position = hit;

							if (ray.hit_side == 0) {
								point.normal = glm::vec3(-ray.step.x, 0.0f, 0.0f);
								point.voxel_coord = glm::vec2(1.0f - frac(hit.z), frac(hit.y));
							}
							else if (ray.hit_side == 1) {
								point.normal = glm::vec3(0.0f, -ray.step.y, 0.0f);
								point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.z));
							}
							else if (ray.hit_side == 2) {
								point.normal = glm::vec3(0.0f, 0.0f, -ray.step.z);
								point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.y));
							}
							return;
						}
						else {
							addStackContext(stack, ray, sub_node);
							++stack.current_level;
							break;
						}
					}
				}
				
				glm::vec3& t_min = context.t_min;
				glm::vec3& t = context.t;

				if (t_min.x < t_min.y) {
					if (t_min.x < t_min.z) {
						context.t_max_min = t_min.x;
						t_min.x += t.x, cell_position.x += ray.step.x, ray.hit_side = 0;
					}
					else {
						context.t_max_min = t_min.z;
						t_min.z += t.z, cell_position.z += ray.step.z, ray.hit_side = 2;
					}
				}
				else {
					if (t_min.y < t_min.z) {
						context.t_max_min = t_min.y;
						t_min.y += t.y, cell_position.y += ray.step.y, ray.hit_side = 1;
					}
					else {
						context.t_max_min = t_min.z;
						t_min.z += t.z, cell_position.z += ray.step.z, ray.hit_side = 2;
					}
				}

				context.check_cell = true;
			}

			--stack.current_level;
		}
	}

	void rec_castRay(Ray& ray, const glm::vec3& position, uint32_t cell_size, const Node<SUB>* node) const
	{
		glm::vec3 cell_pos_i = glm::clamp(glm::ivec3(position.x / cell_size, position.y / cell_size, position.z / cell_size), glm::ivec3(0), glm::ivec3(SUB-1));

		glm::vec3 t_min = ((cell_pos_i + ray.dir) * float(cell_size) - position) / ray.direction;

		const glm::vec3 t = float(cell_size) * ray.t;

		float t_max_min = 0.0f;
		const float t_total = ray.t_total;
		while (checkCell(cell_pos_i)) {
			// Increase pixel complexity
			++ray.point.complexity;
			// We enter the sub node
			const Node<SUB>* sub_node = node->sub[uint32_t(cell_pos_i.x)][uint32_t(cell_pos_i.y)][uint32_t(cell_pos_i.z)];
			if (sub_node) {
				if (sub_node->leaf) {
					HitPoint& point = ray.point;
					const Cell& cell = sub_node->cell;
					const glm::vec3 hit = ray.start + (t_total + t_max_min) * ray.direction;

					point.cell = &cell;
					point.position = hit;

					if (ray.hit_side == 0) {
						point.normal = glm::vec3(-ray.step.x, 0.0f, 0.0f);
						point.voxel_coord = glm::vec2(1.0f - frac(hit.z), frac(hit.y));
					}
					else if (ray.hit_side == 1) {
						point.normal = glm::vec3(0.0f, -ray.step.y, 0.0f);
						point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.z));
					}
					else if (ray.hit_side == 2) {
						point.normal = glm::vec3(0.0f, 0.0f, -ray.step.z);
						point.voxel_coord = glm::vec2(frac(hit.x), frac(hit.y));
					}

					point.distance = t_total + t_max_min;
					return;
				}
				else {
					const uint32_t sub_size = cell_size / SUB;
					const glm::vec3 sub_position = (position + t_max_min * ray.direction) - cell_pos_i * float(cell_size);
					ray.t_total = t_total + t_max_min;
					rec_castRay(ray, sub_position, sub_size, sub_node);
					if (ray.point.cell) {
						return;
					}
				}
			}

			if (t_min.x < t_min.y) {
				if (t_min.x < t_min.z) {
					t_max_min = t_min.x;
					t_min.x += t.x, cell_pos_i.x += ray.step.x, ray.hit_side = 0;
				}
				else {
					t_max_min = t_min.z;
					t_min.z += t.z, cell_pos_i.z += ray.step.z, ray.hit_side = 2;
				}
			}
			else {
				if (t_min.y < t_min.z) {
					t_max_min = t_min.y;
					t_min.y += t.y, cell_pos_i.y += ray.step.y, ray.hit_side = 1;
				}
				else {
					t_max_min = t_min.z;
					t_min.z += t.z, cell_pos_i.z += ray.step.z, ray.hit_side = 2;
				}
			}
		}
	}
};
