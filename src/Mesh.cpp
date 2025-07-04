#include "Mesh.h"

Mesh::Mesh(INT meshId, std::vector<Primitive> primitives)
{
	_id = meshId;
	_primitives = primitives;
}
