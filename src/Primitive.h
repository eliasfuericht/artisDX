#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "AABB.h"

class Primitive
{
public:
	Primitive() = default;
	Primitive(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int32_t materialIndex);
	void BindPrimitiveData(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	MSWRL::ComPtr<ID3D12Resource> CreateBuffer(uint64_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState);
	void UploadBuffers(std::vector<Vertex>& vertices, uint32_t vertexBufferSize, std::vector<uint32_t>& indices, uint32_t indexBufferSize);

	MSWRL::ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};
	uint32_t _vertexCount = 0;

	MSWRL::ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView = {};
	uint32_t _indexCount = 0;

	int32_t _materialIndex = NOTOK;
	AABB _aabb;
};