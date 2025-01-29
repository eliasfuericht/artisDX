#pragma once

#include "pch.h"

#include "Mesh.h"
#include "AABB.h"

class Model
{
public:
	Model() {};
	Model(std::vector<FLOAT> vertices, std::vector<INT> indices);
private:
	Mesh _mesh;
	AABB _aabb;
};