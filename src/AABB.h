#pragma once

#include "pch.h"

#include "D3D12Core.h"

class AABB
{
public:
	AABB() {};
	AABB(const std::vector<Vertex>& vertices);

	void ComputeFromVertices(const std::vector<Vertex>& vertices);
	void Recompute(const XMFLOAT4X4& matrix);

	MSWRL::ComPtr<ID3D12Resource> CreateBuffer(uint64_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState);
	void UploadBuffers();

	void BindMeshData(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	const XMFLOAT3& GetMin() const;
	const XMFLOAT3& GetMax() const;

private:
	XMFLOAT3 _min;
	XMFLOAT3 _max;

	std::vector<Vertex> _aabbVertices;
	MSWRL::ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};

	std::vector<uint32_t> _aabbIndices;
	MSWRL::ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView = {};
	int32_t _indicesSize;
};
