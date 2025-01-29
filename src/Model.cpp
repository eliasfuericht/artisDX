#include "Model.h"

Model::Model(std::vector<FLOAT> vertices, std::vector<INT> indices)
{
	_mesh = Mesh(vertices, indices);
}
