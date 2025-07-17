#pragma once

#include "pch.h"

struct Material 
{

	Material() {};

	std::string _name = "";
	int32_t _baseColorTextureIndex = NOTOK;
	int32_t _normalTextureIndex = NOTOK;
	int32_t _metallicRoughnessTextureIndex = NOTOK;
	int32_t _emissiveTextureIndex = NOTOK;
	int32_t _occlusionTextureIndex = NOTOK;

	// PBR factors
	XMFLOAT4 _baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	float _metallicFactor = 1.0f;
	float _roughnessFactor = 1.0f;

	bool _isTransparent = false;

	fastgltf::AlphaMode _alphaMode;
};