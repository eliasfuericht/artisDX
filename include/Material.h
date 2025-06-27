#pragma once

#include "precompiled/pch.h"

class Material {
public:
	Material() {};
	Material(D3D12_GPU_DESCRIPTOR_HANDLE textureBlockHandle);

	void Bind(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT rootParameterIndex);

	D3D12_GPU_DESCRIPTOR_HANDLE _gpuHandle;
};