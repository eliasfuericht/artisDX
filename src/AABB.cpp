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

	_min = _max = vertices[0].position;

	for (const auto& vertex : vertices)
	{
		const XMFLOAT3& pos = vertex.position;

		_min.x = std::min(_min.x, pos.x);
		_min.y = std::min(_min.y, pos.y);
		_min.z = std::min(_min.z, pos.z);

		_max.x = std::max(_max.x, pos.x);
		_max.y = std::max(_max.y, pos.y);
		_max.z = std::max(_max.z, pos.z);
	}
}

void AABB::UpdateTransform(XMFLOAT4X4 matrix)
{
	XMMATRIX transform = XMLoadFloat4x4(&matrix);

	// Compute the 8 corners of the original AABB
	XMFLOAT3 corners[8] = {
			{ _min.x, _min.y, _min.z }, { _max.x, _min.y, _min.z },
			{ _min.x, _max.y, _min.z }, { _max.x, _max.y, _min.z },
			{ _min.x, _min.y, _max.z }, { _max.x, _min.y, _max.z },
			{ _min.x, _max.y, _max.z }, { _max.x, _max.y, _max.z }
	};

	// Transform all corners and recompute min/max
	XMFLOAT3 newMin = { FLT_MAX, FLT_MAX, FLT_MAX };
	XMFLOAT3 newMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (const auto& corner : corners)
	{
		XMVECTOR point = XMVector3Transform(XMLoadFloat3(&corner), transform);
		XMFLOAT3 transformedPoint;
		XMStoreFloat3(&transformedPoint, point);

		newMin.x = std::min(newMin.x, transformedPoint.x);
		newMin.y = std::min(newMin.y, transformedPoint.y);
		newMin.z = std::min(newMin.z, transformedPoint.z);

		newMax.x = std::max(newMax.x, transformedPoint.x);
		newMax.y = std::max(newMax.y, transformedPoint.y);
		newMax.z = std::max(newMax.z, transformedPoint.z);
	}

	_min = newMin;
	_max = newMax;
}

const XMFLOAT3& AABB::GetMin() const 
{ 
	return _min; 
}

const XMFLOAT3& AABB::GetMax() const
{
	return _max;
}
