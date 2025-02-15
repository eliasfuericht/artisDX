#include "Culler.h"

Culler& Culler::GetInstance()
{
	static Culler instance;
	return instance;
}

void Culler::ExtractPlanes(const XMFLOAT4X4& viewProj)
{
	// TODO:: Extract Planes for culling
}

bool Culler::CheckAABB(const AABB& aabb)
{
	// TODO: Perform check against AABB
	return true;
}
