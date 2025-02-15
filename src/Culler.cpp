#include "Culler.h"

Culler& Culler::GetInstance()
{
	static Culler instance;
	return instance;
}

void Culler::ExtractPlanes(const XMFLOAT4X4& viewProj)
{

	XMMATRIX matViewProj = XMLoadFloat4x4(&viewProj);

	XMVECTOR vp[4] = {
			matViewProj.r[0],
			matViewProj.r[1],
			matViewProj.r[2],
			matViewProj.r[3]
	};

	
	 XMStoreFloat4(&_planes[0], XMPlaneNormalize(vp[3] + vp[0]));
	 XMStoreFloat4(&_planes[1], XMPlaneNormalize(vp[3] - vp[0]));
	 XMStoreFloat4(&_planes[2], XMPlaneNormalize(vp[3] + vp[1]));
	 XMStoreFloat4(&_planes[3], XMPlaneNormalize(vp[3] - vp[1]));
	 XMStoreFloat4(&_planes[4], XMPlaneNormalize(vp[3] + vp[2]));
	 XMStoreFloat4(&_planes[5], XMPlaneNormalize(vp[3] - vp[2]));
}


bool Culler::CheckAABB(const AABB& aabb)
{
	const XMFLOAT3& min = aabb.GetMin();
	const XMFLOAT3& max = aabb.GetMax();

	XMVECTOR minVec = XMVectorSet(min.x, min.y, min.z, 1.0f);
	XMVECTOR maxVec = XMVectorSet(max.x, max.y, max.z, 1.0f);

	for (size_t i = 0; i < 6; ++i)
	{
		const XMVECTOR& plane = XMLoadFloat4(&_planes[i]);

		if ((XMVectorGetX(XMPlaneDot(plane, minVec)) < 0.0f) &&
			(XMVectorGetX(XMPlaneDot(plane, XMVectorSet(max.x, min.y, min.z, 1.0f))) < 0.0f) &&
			(XMVectorGetX(XMPlaneDot(plane, XMVectorSet(min.x, max.y, min.z, 1.0f))) < 0.0f) &&
			(XMVectorGetX(XMPlaneDot(plane, XMVectorSet(max.x, max.y, min.z, 1.0f))) < 0.0f) &&
			(XMVectorGetX(XMPlaneDot(plane, XMVectorSet(min.x, min.y, max.z, 1.0f))) < 0.0f) &&
			(XMVectorGetX(XMPlaneDot(plane, XMVectorSet(max.x, min.y, max.z, 1.0f))) < 0.0f) &&
			(XMVectorGetX(XMPlaneDot(plane, XMVectorSet(min.x, max.y, max.z, 1.0f))) < 0.0f) &&
			(XMVectorGetX(XMPlaneDot(plane, XMVectorSet(max.x, max.y, max.z, 1.0f))) < 0.0f))
		{
			return false;
		}
	}

	return true;
}
