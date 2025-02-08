#pragma once

#include "pch.h"

class AABB
{
public:
	AABB() = default;
	AABB(const std::vector<Vertex>& vertices);

	void ComputeFromVertices(const std::vector<Vertex>& vertices);
	void UpdateTransform(DirectX::XMFLOAT4X4 matrix);

	const DirectX::XMFLOAT3& GetMin() const;
	const DirectX::XMFLOAT3& GetMax() const;

private:
	DirectX::XMFLOAT3 _min;
	DirectX::XMFLOAT3 _max;
};
