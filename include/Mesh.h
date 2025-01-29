#pragma once

#include "pch.h"

class Mesh
{
public:
	Mesh() {};
	Mesh(std::vector<FLOAT> vertices, std::vector<INT> indices);

private:
	std::vector<FLOAT> _vertices;
	std::vector<INT> _indices;
};