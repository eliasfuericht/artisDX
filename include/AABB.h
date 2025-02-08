#pragma once

#include "pch.h"

class AABB
{
public:
	AABB() = default;
	AABB(const std::vector<Vertex>& vertices);

	void ComputeFromVertices(const std::vector<Vertex>& vertices);
	void UpdateTransform(XMFLOAT4X4 matrix);

	const XMFLOAT3& GetMin() const;
	const XMFLOAT3& GetMax() const;

private:
	XMFLOAT3 _min;
	XMFLOAT3 _max;
};
