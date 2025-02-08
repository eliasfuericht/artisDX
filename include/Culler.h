#pragma once

#include "pch.h"

#include "AABB.h"

class Culler
{
public:
	static Culler& GetInstance();

	void ExtractPlanes(const XMFLOAT4X4& viewProj);
	bool CheckAABB(const AABB& aabb);

	Culler(const Culler&) = delete;
	Culler& operator=(const Culler&) = delete;

private:
	Culler() = default;
	~Culler() = default;

	XMFLOAT4 _frustumPlanes[6];
};