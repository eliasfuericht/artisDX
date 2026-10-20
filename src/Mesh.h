#pragma once

#include "pch.h"

#include "Primitive.h"

class Mesh
{
public:
	Mesh() = default;
	Mesh(std::vector<Primitive> primitives);
	uint64_t GetID() const { return _id; }

	uint64_t _id = _nextId++;
	std::string _name = "";
	std::vector<Primitive> _primitives;

private:
	inline static uint64_t _nextId = 0;
};
