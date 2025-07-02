#pragma once

#include "precompiled/pch.h"

class Material {
	std::string name;
	int baseColorTextureIndex = -1;
	int normalTextureIndex = -1;
	int metallicRoughnessTextureIndex = -1;

	// PBR factors
	XMFLOAT4 baseColorFactor = { 1,1,1,1 };
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
};