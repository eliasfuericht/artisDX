#pragma once

#include "pch.h"

#include "GraphicsDevice.h"
#include "DescriptorAllocator.h"
#include "Primitive.h"
#include "AABB.h"

class Mesh
{
public:
	Mesh() {};
	Mesh(INT meshId, std::vector<Primitive> primitives);

	void BindPrimitives(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	INT _id;
	std::string _name;
	std::vector<Primitive> _primitives;

	// weg
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