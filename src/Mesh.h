#pragma once

#include "pch.h"

#include "Primitive.h"

class Mesh
{
public:
	Mesh() {};
	Mesh(INT meshId, std::vector<Primitive> primitives);

	INT _id;
	std::string _name;
	std::vector<Primitive> _primitives;
};