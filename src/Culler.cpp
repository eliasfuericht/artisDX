#include "Culler.h"

Culler& Culler::GetInstance()
{
	static Culler instance;
	return instance;
}

void Culler::ExtractPlanes(const XMFLOAT4X4& viewProj)
{
	XMMATRIX matViewProj = XMLoadFloat4x4(&viewProj);

	// Extract rows of the view-projection matrix
	XMVECTOR row0 = matViewProj.r[0];
	XMVECTOR row1 = matViewProj.r[1];
	XMVECTOR row2 = matViewProj.r[2];
	XMVECTOR row3 = matViewProj.r[3];

	// Extract frustum planes
	XMStoreFloat4(&_planes[0], XMPlaneNormalize(row3 + row2)); // Near
	XMStoreFloat4(&_planes[1], XMPlaneNormalize(row3 - row2)); // Far
	XMStoreFloat4(&_planes[2], XMPlaneNormalize(row3 + row0)); // Left
	XMStoreFloat4(&_planes[3], XMPlaneNormalize(row3 - row0)); // Right
	XMStoreFloat4(&_planes[4], XMPlaneNormalize(row3 + row1)); // Top   <-- Corrected
	XMStoreFloat4(&_planes[5], XMPlaneNormalize(row3 - row1)); // Bottom <-- Corrected
}

bool Culler::CheckAABB(const AABB& aabb, const XMFLOAT4X4& modelMatrix)
{
	const XMFLOAT3& min = aabb.GetMin();
	const XMFLOAT3& max = aabb.GetMax();

	// Load the model matrix into an XMMATRIX
	XMMATRIX modelMat = XMLoadFloat4x4(&modelMatrix);

	// Precompute all 8 corners of the AABB in local space
	XMVECTOR corners[8] = {
			XMVectorSet(min.x, min.y, min.z, 1.0f),
			XMVectorSet(max.x, min.y, min.z, 1.0f),
			XMVectorSet(min.x, max.y, min.z, 1.0f),
			XMVectorSet(max.x, max.y, min.z, 1.0f),
			XMVectorSet(min.x, min.y, max.z, 1.0f),
			XMVectorSet(max.x, min.y, max.z, 1.0f),
			XMVectorSet(min.x, max.y, max.z, 1.0f),
			XMVectorSet(max.x, max.y, max.z, 1.0f)
	};

	// Transform the corners to world space
	for (size_t i = 0; i < 8; ++i)
	{
		corners[i] = XMVector3Transform(corners[i], modelMat);
	}

	// Check each frustum plane
	for (size_t i = 0; i < 6; ++i)
	{
		const XMVECTOR& plane = XMLoadFloat4(&_planes[i]);

		// Check if all corners are outside the plane
		bool allOutside = true;
		for (size_t j = 0; j < 8; ++j)
		{
			if (XMVectorGetX(XMPlaneDotCoord(plane, corners[j])) >= 0.0f)
			{
				// At least one corner is inside the plane
				allOutside = false;
				break;
			}
		}

		// If all corners are outside the plane, the AABB is not visible
		if (allOutside)
		{
			return false;
		}
	}

	// The AABB is inside all planes
	return true;
}