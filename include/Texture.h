#pragma once

#include "precompiled/pch.h"

#include "DirectXTex.h"

class Texture
{
public:
	Texture() {};
	Texture(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, ScratchImage& texture);
	//void BindTextureData(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void CreateGPUHandle(MSWRL::ComPtr<ID3D12Device> device);
	void BindTexture(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);


private:
	void CreateBuffers(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	MSWRL::ComPtr<ID3D12Resource> _textureUploadHeap;
	MSWRL::ComPtr<ID3D12Resource> _textureResource;   
	MSWRL::ComPtr<ID3D12DescriptorHeap> _srvHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE _srvHandle;

	ScratchImage _image;
	ScratchImage _mipMaps;
	UINT _mipCount;
};