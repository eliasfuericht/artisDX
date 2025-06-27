#pragma once

#include "precompiled/pch.h"

#include "DescriptorAllocator.h"
#include "Mesh.h"
#include "AABB.h"

class MeshInstance
{
public:
	MeshInstance(INT MeshInstanceId, Mesh meshInstance, AABB aabbInstance, XMFLOAT4X4 localTransformMatrix, MSWRL::ComPtr<ID3D12Device> device);

	INT id;
	Mesh mesh;
	AABB aabb;
	XMFLOAT4X4 localTransform;

	XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scaling = { 1.0f, 1.0f, 1.0f };

	uint8_t* mappedPtr = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle = {};

private:
	void CreateCBV(MSWRL::ComPtr<ID3D12Device> device);
	
	MSWRL::ComPtr<ID3D12Resource> constantBuffer;
};