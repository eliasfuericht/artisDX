#include "Mesh.h"

Mesh::Mesh(int32_t meshId, std::vector<Primitive> primitives)
{
	_id = meshId;
	_primitives = primitives;
}
