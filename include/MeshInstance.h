#pragma once

#include "precompiled/pch.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"
#include "Mesh.h"
#include "AABB.h"

class MeshInstance
{
public:
	MeshInstance(INT MeshInstanceId, Mesh meshInstance, AABB aabbInstance, XMFLOAT4X4 localTransformMatrix, INT materialIndexInstance);

	INT _id;
	Mesh _mesh;
	AABB _aabb;
	XMFLOAT4X4 _localTransform;
	INT _materialIndex;

	XMFLOAT3 _translation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 _rotation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 _scaling = { 1.0f, 1.0f, 1.0f };

	uint8_t* _mappedPtr = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE _cbvGpuHandle = {};

private:
	void CreateCBV();
	
	MSWRL::ComPtr<ID3D12Resource> _constantBuffer;
};