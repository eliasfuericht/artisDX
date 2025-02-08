#include "AABB.h"

AABB::AABB(const std::vector<Vertex>& vertices)
{
	ComputeFromVertices(vertices);
}

void AABB::ComputeFromVertices(const std::vector<Vertex>& vertices)
{
	if (vertices.empty())
	{
		_min = { 0.0f, 0.0f, 0.0f };
		_max = { 0.0f, 0.0f, 0.0f };
		return;
	}

	// Initialize min/max with first vertex
	_min = _max = vertices[0].position;

	for (const auto& vertex : vertices)
	{
		const DirectX::XMFLOAT3& pos = vertex.position;

		_min.x = std::min(_min.x, pos.x);
		_min.y = std::min(_min.y, pos.y);
		_min.z = std::min(_min.z, pos.z);

		_max.x = std::max(_max.x, pos.x);
		_max.y = std::max(_max.y, pos.y);
		_max.z = std::max(_max.z, pos.z);
	}
}

void AABB::UpdateTransform(DirectX::XMFLOAT4X4 matrix)
{

}

const DirectX::XMFLOAT3& AABB::GetMin() const 
{ 
	return _min; 
}

const DirectX::XMFLOAT3& AABB::GetMax() const
{
	return _max;
}
