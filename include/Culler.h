#pragma once

#include "pch.h"

#include "AABB.h"

class Culler
{
public:
	static Culler& GetInstance();

	void ExtractPlanes(const XMFLOAT4X4& viewProj);
	bool CheckAABB(const AABB& aabb, const XMFLOAT4X4& modelMatrix);

	Culler(const Culler&) = delete;
	Culler& operator=(const Culler&) = delete;

private:
	Culler() = default;
	~Culler() = default;

	std::array<XMFLOAT4, 6> _planes;
};