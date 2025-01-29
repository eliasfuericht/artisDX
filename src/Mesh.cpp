#include "Mesh.h"

Mesh::Mesh(std::vector<FLOAT> vertices, std::vector<INT> indices)
{
	_vertices = vertices;
	_indices = indices;
}