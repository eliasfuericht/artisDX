#pragma once

#include "precompiled/pch.h"

class Material 
{
public:
	Material() {};

	std::string name;
	INT baseColorTextureIndex = NOTOK;
	INT normalTextureIndex = NOTOK;
	INT metallicRoughnessTextureIndex = NOTOK;
	INT emissiveTextureIndex = NOTOK;
	INT occlusionTextureIndex = NOTOK;

	// PBR factors
	XMFLOAT4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	FLOAT metallicFactor = 1.0f;
	FLOAT roughnessFactor = 1.0f;
};