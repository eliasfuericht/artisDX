#pragma once

#include "precompiled/pch.h"

#include "DirectXTex.h"
#include "DescriptorAllocator.h"

class Texture
{
public:
	enum TEXTURETYPE
	{
		ALBEDO = 0,
		NORMAL = 1,
		METAL_AO = 2
	};

public:
	Texture() {};
	Texture(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, ScratchImage& texture);
	void BindTexture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

private:
	void CreateBuffers(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	MSWRL::ComPtr<ID3D12Resource> _textureUploadHeap;
	MSWRL::ComPtr<ID3D12Resource> _textureResource; 
	D3D12_CPU_DESCRIPTOR_HANDLE _srvCpuHandle;

	ScratchImage _image;
	ScratchImage _mipMaps;
	UINT _mipCount;
	Texture::TEXTURETYPE _textureType;
};