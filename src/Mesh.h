#pragma once

#include "pch.h"

#include "Primitive.h"

class Mesh
{
public:
	Mesh() {};
	Mesh(int32_t meshId, std::vector<Primitive> primitives);

	int32_t _id = NOTOK;
	std::string _name = "";
	std::vector<Primitive> _primitives;
};