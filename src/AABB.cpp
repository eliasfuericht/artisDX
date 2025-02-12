#include "AABB.h"

AABB::AABB(const std::vector<Vertex>& vertices)
{
	ComputeFromVertices(vertices);
}

void AABB::ComputeFromVertices(const std::vector<Vertex>& vertices)
{
	if (vertices.empty())
	{
		_localMin = { 0.0f, 0.0f, 0.0f };
		_localMax = { 0.0f, 0.0f, 0.0f };
		return;
	}

	_localMin = _localMax = vertices[0].position;

	for (const auto& vertex : vertices)
	{
		const XMFLOAT3& pos = vertex.position;

		_localMin.x = std::min(_localMin.x, pos.x);
		_localMin.y = std::min(_localMin.y, pos.y);
		_localMin.z = std::min(_localMin.z, pos.z);

		_localMax.x = std::max(_localMax.x, pos.x);
		_localMax.y = std::max(_localMax.y, pos.y);
		_localMax.z = std::max(_localMax.z, pos.z);
	}
}

void AABB::UpdateTransform(const XMFLOAT4X4& matrix)
{
    XMMATRIX transform = XMLoadFloat4x4(&matrix);

    // Compute the 8 corners of the original local-space AABB
    XMFLOAT3 corners[8] = {
        { _localMin.x, _localMin.y, _localMin.z }, { _localMax.x, _localMin.y, _localMin.z },
        { _localMin.x, _localMax.y, _localMin.z }, { _localMax.x, _localMax.y, _localMin.z },
        { _localMin.x, _localMin.y, _localMax.z }, { _localMax.x, _localMin.y, _localMax.z },
        { _localMin.x, _localMax.y, _localMax.z }, { _localMax.x, _localMax.y, _localMax.z }
    };

    // Transform all corners and recompute world-space min/max
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
