#include "Primitive.h"

Primitive::Primitive(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, int32_t materialIndex)
{
	_vertexCount = vertices.size();
	uint32_t vertexBufferSize = vertices.size() * sizeof(Vertex);
	_vertexBuffer = CreateBuffer(vertexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	_vertexBuffer->SetName(L"VertexBufferResource");

	_indexCount = indices.size();
	uint32_t indexBufferSize = _indexCount * sizeof(uint32_t);
	_indexBuffer = CreateBuffer(indexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	_indexBuffer->SetName(L"IndexBufferResource");

	UploadBuffers(vertices, vertexBufferSize, indices, indexBufferSize);

	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.SizeInBytes = vertexBufferSize;
	_vertexBufferView.StrideInBytes = sizeof(Vertex);

	_indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_indexBufferView.SizeInBytes = indexBufferSize;
	_indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	_materialIndex = materialIndex;
	_aabb = AABB(vertices);
}

MSWRL::ComPtr<ID3D12Resource> Primitive::CreateBuffer(uint64_t size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState)
{
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = heapType;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = size;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	MSWRL::ComPtr<ID3D12Resource> buffer;

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&buffer)
	));

	return buffer;
}

void Primitive::UploadBuffers(std::vector<Vertex>& vertices, uint32_t vertexBufferSize, std::vector<uint32_t>& indices, uint32_t indexBufferSize)
{
	// Upload vertex data
	uint8_t* pVertexDataBegin = nullptr;
	D3D12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, vertices.data(), vertexBufferSize);
	_vertexBuffer->Unmap(0, nullptr);

	// Upload index data
	void* pIndexDataBegin = nullptr;
	ThrowIfFailed(_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	memcpy(pIndexDataBegin, indices.data(), indexBufferSize);
	_indexBuffer->Unmap(0, nullptr);
}

void Primitive::BindPrimitiveData(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
	commandList->IASetIndexBuffer(&_indexBufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(_indexCount, 1, 0, 0, 0);
}
