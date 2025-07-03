#pragma once

#include "pch.h"

struct Material 
{
	Material() {};

	std::string _name;
	INT _baseColorTextureIndex = NOTOK;
	INT _normalTextureIndex = NOTOK;
	INT _metallicRoughnessTextureIndex = NOTOK;
	INT _emissiveTextureIndex = NOTOK;
	INT _occlusionTextureIndex = NOTOK;

	// PBR factors
	XMFLOAT4 _baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	FLOAT _metallicFactor = 1.0f;
	FLOAT _roughnessFactor = 1.0f;
};