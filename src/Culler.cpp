#include "Culler.h"

Culler& Culler::GetInstance()
{
	static Culler instance;
	return instance;
}

void Culler::ExtractPlanes(const XMFLOAT4X4& viewProj)
{
	_frustumPlanes[0] = XMFLOAT4(
		viewProj.m[3][0] + viewProj.m[0][0],
		viewProj.m[3][1] + viewProj.m[0][1],
		viewProj.m[3][2] + viewProj.m[0][2],
		viewProj.m[3][3] + viewProj.m[0][3] 
	);

	// Right plane
	_frustumPlanes[1] = XMFLOAT4(
		viewProj.m[3][0] - viewProj.m[0][0],
		viewProj.m[3][1] - viewProj.m[0][1],
		viewProj.m[3][2] - viewProj.m[0][2],
		viewProj.m[3][3] - viewProj.m[0][3]
	);

	// Top plane
	_frustumPlanes[2] = XMFLOAT4(
		viewProj.m[3][0] - viewProj.m[1][0],
		viewProj.m[3][1] - viewProj.m[1][1],
		viewProj.m[3][2] - viewProj.m[1][2],
		viewProj.m[3][3] - viewProj.m[1][3]
	);

	// Bottom plane
	_frustumPlanes[3] = XMFLOAT4(
		viewProj.m[3][0] + viewProj.m[1][0],
		viewProj.m[3][1] + viewProj.m[1][1],
		viewProj.m[3][2] + viewProj.m[1][2],
		viewProj.m[3][3] + viewProj.m[1][3]
	);

	// Near plane
	_frustumPlanes[4] = XMFLOAT4(
		viewProj.m[3][0] + viewProj.m[2][0],
		viewProj.m[3][1] + viewProj.m[2][1],
		viewProj.m[3][2] + viewProj.m[2][2],
		viewProj.m[3][3] + viewProj.m[2][3]
	);

	// Far plane
	_frustumPlanes[5] = XMFLOAT4(
		viewProj.m[3][0] - viewProj.m[2][0],
		viewProj.m[3][1] - viewProj.m[2][1],
		viewProj.m[3][2] - viewProj.m[2][2],
		viewProj.m[3][3] - viewProj.m[2][3]
	);

	// Normalize all planes
	for (auto& plane : _frustumPlanes)
	{
		XMVECTOR planeVec = XMLoadFloat4(&plane);
		planeVec = XMVector3Normalize(planeVec);
		XMStoreFloat4(&plane, planeVec);
	}
}

bool Culler::CheckAABB(const AABB& aabb)
{
	// Test all 6 planes of the frustum
	for (const auto& plane : _frustumPlanes)
	{
		// Transform AABB min/max to check against the plane
		XMFLOAT3 min = aabb.GetMin();
		XMFLOAT3 max = aabb.GetMax();

		XMVECTOR planeNormal = XMLoadFloat4(&plane);
		XMFLOAT3 vertices[8] = {
				{ min.x, min.y, min.z },
				{ max.x, min.y, min.z },
				{ min.x, max.y, min.z },
				{ max.x, max.y, min.z },
				{ min.x, min.y, max.z },
				{ max.x, min.y, max.z },
				{ min.x, max.y, max.z },
				{ max.x, max.y, max.z }
		};

		bool inside = false;

		// Test if at least one vertex is inside the plane
		for (const auto& vertex : vertices)
		{
			XMVECTOR point = XMLoadFloat3(&vertex);
			float distance = XMVectorGetX(XMPlaneDotCoord(planeNormal, point));

			if (distance >= 0)
			{
				inside = true;
				break; // At least one point is inside
			}
		}

		if (!inside)
		{
			return false; // If no points are inside, the AABB is outside
		}
	}

	return true; // AABB is inside the frustum
}