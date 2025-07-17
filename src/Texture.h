#pragma once

#include "pch.h"

#include <wincodec.h>

#include "DirectXTex.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"

#include "ShaderPass.h"

class Texture
{
public:
	enum TEXTURETYPE
	{
		TEXTURE_ALBEDO = 0,
		TEXTURE_METALLICROUGHNESS = 1,
		TEXTURE_NORMAL = 2,
		TEXTURE_EMISSIVE = 3,
		TEXTURE_OCCLUSION = 4
	};

public:
	Texture() {};
	Texture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, Texture::TEXTURETYPE texType, ScratchImage& scratchImage);
	void BindTexture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, const ShaderPass& shaderPass);

private:
	void CreateBuffers(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	MSWRL::ComPtr<ID3D12Resource> _textureUploadHeap;
	MSWRL::ComPtr<ID3D12Resource> _textureResource; 
	D3D12_CPU_DESCRIPTOR_HANDLE _srvCpuHandle;

	ScratchImage _image;
	uint32_t _mipCount;
	Texture::TEXTURETYPE _textureType;
};