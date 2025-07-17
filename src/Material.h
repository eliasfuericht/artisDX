#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"
#include "ShaderPass.h"

class Material 
{
public:
	Material();

	std::string _name = "";
	int32_t _baseColorTextureIndex = NOTOK;
	int32_t _normalTextureIndex = NOTOK;
	int32_t _metallicRoughnessTextureIndex = NOTOK;
	int32_t _emissiveTextureIndex = NOTOK;
	int32_t _occlusionTextureIndex = NOTOK;

	struct PBRFactors
	{
		XMFLOAT4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
	};

	PBRFactors _pbrFactors;

	fastgltf::AlphaMode _alphaMode;

	uint8_t* _mappedMaterialFactorsPtr = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE _cbvMaterialFactorsGpuHandle = {};
	MSWRL::ComPtr<ID3D12Resource> _MaterialFactorsBufferResource;

	void CreateCBV();

	void BindMaterialFactorsData(const ShaderPass& shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
};