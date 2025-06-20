#pragma once

#include "precompiled/pch.h"

#include "AABB.h"
#include "Model.h"

class FrustumCuller
{
public:
	static FrustumCuller& GetInstance();

	void ExtractPlanes(const XMFLOAT4X4& viewProj, MSWRL::ComPtr<ID3D12Device> device);
	bool CheckAABB(const AABB& aabb, const XMFLOAT4X4& modelMatrix);
	void BindMeshData(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void CreateModelMatrixBuffer(MSWRL::ComPtr<ID3D12Device> device);

	FrustumCuller(const FrustumCuller&) = delete;
	FrustumCuller& operator=(const FrustumCuller&) = delete;

private:
	XMVECTOR IntersectPlanes(const XMFLOAT4& p1, const XMFLOAT4& p2, const XMFLOAT4& p3);
	void ExtractFrustumVertices();

	MSWRL::ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* device, UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState);

	void UploadBuffers();


	FrustumCuller() = default;
	~FrustumCuller() = default;

	std::array<XMFLOAT4, 6> _planes;

	std::vector<Vertex> _frustumVertices;
	MSWRL::ComPtr<ID3D12Resource> _vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView = {};

	std::vector<uint32_t> _frustumIndices;
	MSWRL::ComPtr<ID3D12Resource> _indexBuffer;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView = {};
	size_t _indicesSize;

	XMFLOAT4X4 _modelMatrix;

	MSWRL::ComPtr<ID3D12Resource> _modelMatrixBuffer;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _modelMatrixBufferHeap;

	UINT8* _mappedUniformBuffer;
};