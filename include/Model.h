#pragma once

#include "pch.h"

#include "Mesh.h"
#include "AABB.h"

class Model
{
public:
	Model() {};
	Model(MSWRL::ComPtr<ID3D12Device> device, std::vector<Vertex> vertices, std::vector<uint32_t> indices, DirectX::XMFLOAT4X4 modelMatrix);
	void DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void Transform(FLOAT x);

private:
	void CreateModelMatrixBuffer(MSWRL::ComPtr<ID3D12Device> device);

	Mesh _mesh;
	AABB _aabb;
	DirectX::XMFLOAT4X4 _modelMatrix;
	MSWRL::ComPtr<ID3D12Resource> _modelMatrixBuffer;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _modelMatrixBufferHeap;

	UINT8* _mappedUniformBuffer;
};