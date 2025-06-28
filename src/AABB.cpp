#include "AABB.h"

AABB::AABB(const std::vector<Vertex>& vertices)
{
	ComputeFromVertices(vertices);
}

void AABB::ComputeFromVertices(const std::vector<Vertex>& vertices)
{
	XMFLOAT3 min = vertices[0].position;
	XMFLOAT3 max = vertices[0].position;

	for (size_t i = 1; i < vertices.size(); ++i) {
		const XMFLOAT3& current = vertices[i].position;
		min.x = std::min(min.x, current.x);
		min.y = std::min(min.y, current.y);
		min.z = std::min(min.z, current.z);

		max.x = std::max(max.x, current.x);
		max.y = std::max(max.y, current.y);
		max.z = std::max(max.z, current.z);
	}

	_min = min;
	_max = max;

	_aabbVertices = {
			{{_min.x, _min.y, _min.z}, {0, 1, 0}, {1, 0, 0, 1}, {0, 0}}, 
			{{_min.x, _max.y, _min.z}, {0, 1, 0}, {1, 0, 0, 1}, {1, 1}}, 
			{{_min.x, _min.y, _max.z}, {0, 1, 0}, {1, 0, 0, 1}, {0, 1}}, 
			{{_min.x, _max.y, _max.z}, {0, 1, 0}, {1, 0, 0, 1}, {1, 0}}, 
			{{_max.x, _min.y, _min.z}, {0, 1, 0}, {1, 0, 0, 1}, {0, 0}}, 
			{{_max.x, _max.y, _min.z}, {0, 1, 0}, {1, 0, 0, 1}, {1, 1}}, 
			{{_max.x, _min.y, _max.z}, {0, 1, 0}, {1, 0, 0, 1}, {0, 1}}, 
			{{_max.x, _max.y, _max.z}, {0, 1, 0}, {1, 0, 0, 1}, {1, 0}}	 
	};

	_aabbIndices = {
		0, 1, 2, 1, 3, 2, 
		4, 5, 6, 5, 7, 6, 
		0, 2, 4, 2, 6, 4, 
		1, 5, 3, 5, 7, 3, 
		0, 4, 1, 4, 5, 1, 
		2, 3, 6, 3, 7, 6  
	};

	UINT vertexBufferSize = _aabbVertices.size() * sizeof(Vertex);
	_vertexBuffer = CreateBuffer(vertexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

	_indicesSize = _aabbIndices.size();
	UINT indexBufferSize = _indicesSize * sizeof(uint32_t);
	_indexBuffer = CreateBuffer(indexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

	UploadBuffers();

	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.SizeInBytes = vertexBufferSize;
	_vertexBufferView.StrideInBytes = sizeof(Vertex);

	_indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_indexBufferView.SizeInBytes = indexBufferSize;
	_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

void AABB::Recompute(const XMFLOAT4X4& matrix)
{
	XMFLOAT3 corners[8];

	XMMATRIX matTransform = XMLoadFloat4x4(&matrix);

	XMVECTOR points[8] = {
			XMVectorSet(_min.x, _min.y, _min.z, 1.0f),
			XMVectorSet(_min.x, _max.y, _min.z, 1.0f),
			XMVectorSet(_min.x, _min.y, _max.z, 1.0f),
			XMVectorSet(_min.x, _max.y, _max.z, 1.0f),
			XMVectorSet(_max.x, _min.y, _min.z, 1.0f),
			XMVectorSet(_max.x, _max.y, _min.z, 1.0f),
			XMVectorSet(_max.x, _min.y, _max.z, 1.0f),
			XMVectorSet(_max.x, _max.y, _max.z, 1.0f),
	};

	for (int i = 0; i < 8; ++i) {
		XMVECTOR transformed = XMVector4Transform(points[i], matTransform);
		XMStoreFloat3(&corners[i], transformed);
	}

	XMFLOAT3 newMin = corners[0];
	XMFLOAT3 newMax = corners[0];

	for (size_t i = 1; i < 8; ++i) {
		const XMFLOAT3& current = corners[i];
		newMin.x = std::min(newMin.x, current.x);
		newMin.y = std::min(newMin.y, current.y);
		newMin.z = std::min(newMin.z, current.z);
		
		newMax.x = std::max(newMax.x, current.x);
		newMax.y = std::max(newMax.y, current.y);
		newMax.z = std::max(newMax.z, current.z);
	}

	_min = newMin;
	_max = newMax;
}

MSWRL::ComPtr<ID3D12Resource> AABB::CreateBuffer(UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState)
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

	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&buffer)
	));

	return buffer;
}

void AABB::UploadBuffers()
{
	// Map vertex buffer and copy data
	void* mappedData = nullptr;
	_vertexBuffer->Map(0, nullptr, &mappedData);
	UINT vertexBufferSize = _aabbVertices.size() * sizeof(Vertex);
	memcpy(mappedData, _aabbVertices.data(), vertexBufferSize);
	_vertexBuffer->Unmap(0, nullptr);

	// Map index buffer and copy data
	_indexBuffer->Map(0, nullptr, &mappedData);
	UINT indexBufferSize = _aabbIndices.size() * sizeof(uint32_t);

	memcpy(mappedData, _aabbIndices.data(), indexBufferSize);
	_indexBuffer->Unmap(0, nullptr);
}


const XMFLOAT3& AABB::GetMin() const 
{ 
	return _min; 
}

const XMFLOAT3& AABB::GetMax() const
{
	return _max;
}

void AABB::BindMeshData(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
	commandList->IASetIndexBuffer(&_indexBufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(_indicesSize, 1, 0, 0, 0);
}