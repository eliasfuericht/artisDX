#pragma once

#include "precompiled/pch.h"

#include <wincodec.h>

#include "DirectXTex.h"

#include "GraphicsDevice.h"
#include "DescriptorAllocator.h"

class Texture
{
public:
	enum TEXTURETYPE
	{
		ALBEDO = 0,
		METALLICROUGHNESS = 1,
		NORMAL = 2,
		EMISSIVE = 3,
		OCCLUSION = 4
	};

public:
	Texture() {};
	Texture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, Texture::TEXTURETYPE texType, ScratchImage& scratchImage);
	void BindTexture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

private:
	void CreateBuffers(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	MSWRL::ComPtr<ID3D12Resource> _textureUploadHeap;
	MSWRL::ComPtr<ID3D12Resource> _textureResource; 
	D3D12_CPU_DESCRIPTOR_HANDLE _srvCpuHandle;

	ScratchImage _image;
	UINT _mipCount;
	Texture::TEXTURETYPE _textureType;
};