#pragma once

#include "pch.h"

class Mesh
{
public:
	Mesh() {};
	Mesh(MSWRL::ComPtr<ID3D12Device> device, std::vector<Vertex> vertices, std::vector<uint32_t> indices);
	void BindMeshData(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	~Mesh();

private:
	MSWRL::ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* device, UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState);
	void UploadBuffers(std::vector<Vertex> vertices, UINT vertexBufferSize, std::vector<uint32_t> indices, UINT indexBufferSize);

	MSWRL::ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};

	MSWRL::ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView = {};
	size_t _indicesSize;
};