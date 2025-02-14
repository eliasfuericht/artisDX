#include "Culler.h"

Culler& Culler::GetInstance()
{
	static Culler instance;
	return instance;
}

void Culler::ExtractPlanes(const XMFLOAT4X4& viewProj)
{
	// Load the matrix
	XMMATRIX m = XMLoadFloat4x4(&viewProj);

	// Extract planes (left-handed coordinate system)
	XMVECTOR row1 = m.r[0];
	XMVECTOR row2 = m.r[1];
	XMVECTOR row3 = m.r[2];
	XMVECTOR row4 = m.r[3];

	// Left plane: row4 + row1
	XMStoreFloat4(&_planes[0], XMPlaneNormalize(row4 + row1));
	// Right plane: row4 - row1
	XMStoreFloat4(&_planes[1], XMPlaneNormalize(row4 - row1));
	// Bottom plane: row4 + row2
	XMStoreFloat4(&_planes[2], XMPlaneNormalize(row4 + row2));
	// Top plane: row4 - row2
	XMStoreFloat4(&_planes[3], XMPlaneNormalize(row4 - row2));
	// Near plane: row4 + row3
	XMStoreFloat4(&_planes[4], XMPlaneNormalize(row4 + row3));
	// Far plane: row4 - row3
	XMStoreFloat4(&_planes[5], XMPlaneNormalize(row4 - row3));
}

bool Culler::CheckAABB(const AABB& aabb, const XMFLOAT4X4& modelMatrix)
{
	// Transform AABB to world space
	XMFLOAT3 localMin = aabb.GetMin();
	XMFLOAT3 localMax = aabb.GetMax();

	XMVECTOR corners[8] = {
			XMVectorSet(localMin.x, localMin.y, localMin.z, 1.0f),
			XMVectorSet(localMax.x, localMin.y, localMin.z, 1.0f),
			XMVectorSet(localMin.x, localMax.y, localMin.z, 1.0f),
			XMVectorSet(localMax.x, localMax.y, localMin.z, 1.0f),
			XMVectorSet(localMin.x, localMin.y, localMax.z, 1.0f),
			XMVectorSet(localMax.x, localMin.y, localMax.z, 1.0f),
			XMVectorSet(localMin.x, localMax.y, localMax.z, 1.0f),
			XMVectorSet(localMax.x, localMax.y, localMax.z, 1.0f)
	};

	XMMATRIX transform = XMLoadFloat4x4(&modelMatrix);
	XMVECTOR worldMin = XMVector3Transform(corners[0], transform);
	XMVECTOR worldMax = worldMin;

	for (int i = 1; i < 8; ++i)
	{
		XMVECTOR transformedCorner = XMVector3Transform(corners[i], transform);
		worldMin = XMVectorMin(worldMin, transformedCorner);
		worldMax = XMVectorMax(worldMax, transformedCorner);
	}

	XMFLOAT3 min, max;
	XMStoreFloat3(&min, worldMin);
	XMStoreFloat3(&max, worldMax);

	// Perform frustum check in world space
	for (const auto& plane : _planes)
	{
		XMVECTOR normal = XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(&plane));
		float d = plane.w;

		// Compute positive and negative vertex of the transformed AABB relative to the plane normal
		XMFLOAT3 positive, negative;

		positive.x = (plane.x >= 0.0f) ? max.x : min.x;
		positive.y = (plane.y >= 0.0f) ? max.y : min.y;
		positive.z = (plane.z >= 0.0f) ? max.z : min.z;

		negative.x = (plane.x >= 0.0f) ? min.x : max.x;
		negative.y = (plane.y >= 0.0f) ? min.y : max.y;
		negative.z = (plane.z >= 0.0f) ? min.z : max.z;

		// Check if the AABB is completely outside the plane
		if (XMVectorGetX(XMVector3Dot(XMLoadFloat3(&positive), normal)) + d < 0.0f)
		{
			return false; // AABB is completely outside this plane
		}
	}

	return true; // AABB is at least partially inside the frustum
}
