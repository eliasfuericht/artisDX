#include "AABB.h"

AABB::AABB(MSWRL::ComPtr<ID3D12Device> device, const std::vector<Vertex>& vertices)
{
	ComputeFromVertices(device, vertices);
}

void AABB::ComputeFromVertices(MSWRL::ComPtr<ID3D12Device> device, const std::vector<Vertex>& vertices)
{
	if (vertices.empty())
	{
		_localMin = { 0.0f, 0.0f, 0.0f };
		_localMax = { 0.0f, 0.0f, 0.0f };
		return;
	}

	_localMin = _localMax = vertices[0].position;

	for (const auto& vertex : vertices)
	{
		const XMFLOAT3& pos = vertex.position;

		_localMin.x = std::min(_localMin.x, pos.x);
		_localMin.y = std::min(_localMin.y, pos.y);
		_localMin.z = std::min(_localMin.z, pos.z);

		_localMax.x = std::max(_localMax.x, pos.x);
		_localMax.y = std::max(_localMax.y, pos.y);
		_localMax.z = std::max(_localMax.z, pos.z);
	}

	_aabbVertices = {
			{{_localMin.x, _localMin.y, _localMin.z}, {0, 1, 0}, {1, 0, 0, 1}, {0, 0}},
			{{_localMax.x, _localMax.y, _localMax.z}, {0, 1, 0}, {1, 0, 0, 1}, {1, 1}},
			{{_localMin.x, _localMin.y, _localMax.z}, {0, 1, 0}, {1, 0, 0, 1}, {0, 1}},
			{{_localMin.x, _localMax.y, _localMin.z}, {0, 1, 0}, {1, 0, 0, 1}, {1, 0}},
			{{_localMax.x, _localMin.y, _localMin.z}, {0, 1, 0}, {1, 0, 0, 1}, {0, 0}},
			{{_localMin.x, _localMax.y, _localMax.z}, {0, 1, 0}, {1, 0, 0, 1}, {1, 1}},
			{{_localMax.x, _localMin.y, _localMax.z}, {0, 1, 0}, {1, 0, 0, 1}, {0, 1}},
			{{_localMax.x, _localMax.y, _localMin.z}, {0, 1, 0}, {1, 0, 0, 1}, {1, 0}}
	};

	_aabbIndices = {
		// Bottom face edges
		0, 1, 1, 3, 3, 2, 2, 0,

		// Top face edges
		4, 5, 5, 7, 7, 6, 6, 4,

		// Vertical edges connecting top and bottom
		0, 4, 1, 5, 3, 7, 2, 6
	};



	UINT vertexBufferSize = _aabbVertices.size() * sizeof(Vertex);
	_vertexBuffer = CreateBuffer(device.Get(), vertexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

	_indicesSize = _aabbIndices.size();
	UINT indexBufferSize = _indicesSize * sizeof(uint32_t);
	_indexBuffer = CreateBuffer(device.Get(), indexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

	UploadBuffers();

	_vertexBufferView.BufferLocation = _vertexBuffer->GetGPUVirtualAddress();
	_vertexBufferView.SizeInBytes = vertexBufferSize;
	_vertexBufferView.StrideInBytes = sizeof(Vertex);

	_indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
	_indexBufferView.SizeInBytes = indexBufferSize;
	_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
}

MSWRL::ComPtr<ID3D12Resource> AABB::CreateBuffer(ID3D12Device* device, UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState)
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

	ThrowIfFailed(device->CreateCommittedResource(
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


void AABB::UpdateTransform(const XMFLOAT4X4& matrix)
{
    XMMATRIX transform = XMLoadFloat4x4(&matrix);

    // Compute the 8 corners of the original local-space AABB
    XMFLOAT3 corners[8] = {
        { _localMin.x, _localMin.y, _localMin.z }, { _localMax.x, _localMin.y, _localMin.z },
        { _localMin.x, _localMax.y, _localMin.z }, { _localMax.x, _localMax.y, _localMin.z },
        { _localMin.x, _localMin.y, _localMax.z }, { _localMax.x, _localMin.y, _localMax.z },
        { _localMin.x, _localMax.y, _localMax.z }, { _localMax.x, _localMax.y, _localMax.z }
    };

    // Transform all corners and recompute world-space min/max
    XMFLOAT3 newMin = { FLT_MAX, FLT_MAX, FLT_MAX };
    XMFLOAT3 newMax = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

    for (const auto& corner : corners)
    {
        XMVECTOR point = XMVector3Transform(XMLoadFloat3(&corner), transform);
        XMFLOAT3 transformedPoint;
        XMStoreFloat3(&transformedPoint, point);

        newMin.x = std::min(newMin.x, transformedPoint.x);
        newMin.y = std::min(newMin.y, transformedPoint.y);
        newMin.z = std::min(newMin.z, transformedPoint.z);

        newMax.x = std::max(newMax.x, transformedPoint.x);
        newMax.y = std::max(newMax.y, transformedPoint.y);
        newMax.z = std::max(newMax.z, transformedPoint.z);
    }

    _min = newMin;
    _max = newMax;
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
	commandList->IASetVertexBuffers(0, 1, &_vertexBufferView); // Slot 0, 1 buffer
	commandList->IASetIndexBuffer(&_indexBufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(_indicesSize, 1, 0, 0, 0);
}