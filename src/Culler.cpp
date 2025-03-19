#include "Culler.h"

Culler& Culler::GetInstance()
{
	static Culler instance;
	return instance;
}

void Culler::ExtractPlanes(const XMFLOAT4X4& viewProj, MSWRL::ComPtr<ID3D12Device> device)
{
	XMMATRIX matViewProj = XMLoadFloat4x4(&viewProj);

	// Extract rows of the view-projection matrix
	XMVECTOR row0 = matViewProj.r[0];
	XMVECTOR row1 = matViewProj.r[1];
	XMVECTOR row2 = matViewProj.r[2];
	XMVECTOR row3 = matViewProj.r[3];

	// Extract frustum planes
	XMStoreFloat4(&_planes[0], XMPlaneNormalize(row3 + row2));
	XMStoreFloat4(&_planes[1], XMPlaneNormalize(row3 - row2));
	XMStoreFloat4(&_planes[2], XMPlaneNormalize(row3 + row0));
	XMStoreFloat4(&_planes[3], XMPlaneNormalize(row3 - row0));
	XMStoreFloat4(&_planes[4], XMPlaneNormalize(row3 + row1));
	XMStoreFloat4(&_planes[5], XMPlaneNormalize(row3 - row1));

	ExtractFrustumVertices();

	UINT vertexBufferSize = _frustumVertices.size() * sizeof(Vertex);
	_vertexBuffer = CreateBuffer(device.Get(), vertexBufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);

	_indicesSize = _frustumIndices.size();
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

void Culler::ExtractFrustumVertices()
{
	// lambda to extract intersectionpoints
	auto IntersectPlanes = [](const XMFLOAT4& p1, const XMFLOAT4& p2, const XMFLOAT4& p3) -> XMFLOAT3 {
		XMVECTOR p1v = XMLoadFloat4(&p1);
		XMVECTOR p2v = XMLoadFloat4(&p2);
		XMVECTOR p3v = XMLoadFloat4(&p3);

		XMVECTOR n1 = XMVectorSet(p1.x, p1.y, p1.z, 0.0f);
		XMVECTOR n2 = XMVectorSet(p2.x, p2.y, p2.z, 0.0f);
		XMVECTOR n3 = XMVectorSet(p3.x, p3.y, p3.z, 0.0f);

		XMVECTOR d1 = XMVectorReplicate(-p1.w);
		XMVECTOR d2 = XMVectorReplicate(-p2.w);
		XMVECTOR d3 = XMVectorReplicate(-p3.w);

		XMVECTOR n2n3 = XMVector3Cross(n2, n3);
		XMVECTOR n3n1 = XMVector3Cross(n3, n1);
		XMVECTOR n1n2 = XMVector3Cross(n1, n2);

		XMVECTOR denom = XMVector3Dot(n1, n2n3);
		XMVECTOR point = (d1 * n2n3 + d2 * n3n1 + d3 * n1n2) / denom;

		XMFLOAT3 result;
		XMStoreFloat3(&result, point);
		return result;
		};

	_frustumVertices.resize(8);
	_frustumVertices[0] = { IntersectPlanes(_planes[0], _planes[2], _planes[4]), {0, 1, 0}, {1, 0, 0, 1}, {0, 0} };
	_frustumVertices[1] = { IntersectPlanes(_planes[0], _planes[3], _planes[4]), {0, 1, 0}, {1, 0, 0, 1}, {1, 1} };
	_frustumVertices[2] = { IntersectPlanes(_planes[0], _planes[2], _planes[5]), {0, 1, 0}, {1, 0, 0, 1}, {0, 1} };
	_frustumVertices[3] = { IntersectPlanes(_planes[0], _planes[3], _planes[5]), {0, 1, 0}, {1, 0, 0, 1}, {1, 0} };
	_frustumVertices[4] = { IntersectPlanes(_planes[1], _planes[2], _planes[4]), {0, 1, 0}, {1, 0, 0, 1}, {0, 0} };
	_frustumVertices[5] = { IntersectPlanes(_planes[1], _planes[3], _planes[4]), {0, 1, 0}, {1, 0, 0, 1}, {1, 1} };
	_frustumVertices[6] = { IntersectPlanes(_planes[1], _planes[2], _planes[5]), {0, 1, 0}, {1, 0, 0, 1}, {0, 1} };
	_frustumVertices[7] = { IntersectPlanes(_planes[1], _planes[3], _planes[5]), {0, 1, 0}, {1, 0, 0, 1}, {1, 0} };

	// indices (thank you deepseek)
	_frustumIndices = {
		0, 1, 2, 2, 1, 3,
		4, 5, 6, 6, 5, 7,
		0, 2, 4, 4, 2, 6,
		1, 3, 5, 5, 3, 7,
		0, 4, 1, 1, 4, 5,
		2, 6, 3, 3, 6, 7
	};
}

MSWRL::ComPtr<ID3D12Resource> Culler::CreateBuffer(ID3D12Device* device, UINT64 size, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState)
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

void Culler::CreateModelMatrixBuffer(MSWRL::ComPtr<ID3D12Device> device)
{
	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_modelMatrixBufferHeap)));

	D3D12_RESOURCE_DESC uboResourceDesc;
	uboResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uboResourceDesc.Alignment = 0;
	uboResourceDesc.Width = (sizeof(_modelMatrix) + 255) & ~255;
	uboResourceDesc.Height = 1;
	uboResourceDesc.DepthOrArraySize = 1;
	uboResourceDesc.MipLevels = 1;
	uboResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	uboResourceDesc.SampleDesc.Count = 1;
	uboResourceDesc.SampleDesc.Quality = 0;
	uboResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uboResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &uboResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&_modelMatrixBuffer)));
	_modelMatrixBufferHeap->SetName(L"Model Matrix Buffer Upload Resource Heap");

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _modelMatrixBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (sizeof(_modelMatrix) + 255) & ~255; // CB size is required to be 256-byte aligned.

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(_modelMatrixBufferHeap->GetCPUDescriptorHandleForHeapStart());
	cbvHandle.ptr = cbvHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

	device->CreateConstantBufferView(&cbvDesc, cbvHandle);

	// Map once and keep the pointer
	D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(_modelMatrixBuffer->Map(0, &readRange, reinterpret_cast<void**>(&_mappedUniformBuffer)));
	memcpy(_mappedUniformBuffer, &_modelMatrix, sizeof(_modelMatrix));
	_modelMatrixBuffer->Unmap(0, &readRange);
}

void Culler::UploadBuffers()
{
	// Map vertex buffer and copy data
	void* mappedData = nullptr;
	_vertexBuffer->Map(0, nullptr, &mappedData);
	UINT vertexBufferSize = _frustumVertices.size() * sizeof(Vertex);
	memcpy(mappedData, _frustumVertices.data(), vertexBufferSize);
	_vertexBuffer->Unmap(0, nullptr);

	// Map index buffer and copy data
	_indexBuffer->Map(0, nullptr, &mappedData);
	UINT indexBufferSize = _frustumIndices.size() * sizeof(uint32_t);

	memcpy(mappedData, _frustumIndices.data(), indexBufferSize);
	_indexBuffer->Unmap(0, nullptr);
}

void Culler::BindMeshData(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// identity matrix
	auto identityMatrix = XMMatrixIdentity();
	memcpy(_mappedUniformBuffer, &identityMatrix, sizeof(identityMatrix));

	commandList->SetGraphicsRootConstantBufferView(1, _modelMatrixBuffer->GetGPUVirtualAddress());

	commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
	commandList->IASetIndexBuffer(&_indexBufferView);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->DrawIndexedInstanced(_indicesSize, 1, 0, 0, 0);
}

bool Culler::CheckAABB(const AABB& aabb, const XMFLOAT4X4& modelMatrix)
{
	const XMFLOAT3& min = aabb.GetMin();
	const XMFLOAT3& max = aabb.GetMax();

	// Load the model matrix into an XMMATRIX
	XMMATRIX modelMat = XMLoadFloat4x4(&modelMatrix);

	// Precompute all 8 corners of the AABB in local space
	XMVECTOR corners[8] = {
			XMVectorSet(min.x, min.y, min.z, 1.0f),
			XMVectorSet(min.x, max.y, min.z, 1.0f),
			XMVectorSet(min.x, min.y, max.z, 1.0f),
			XMVectorSet(min.x, max.y, max.z, 1.0f),
			XMVectorSet(max.x, min.y, min.z, 1.0f),
			XMVectorSet(max.x, max.y, min.z, 1.0f),
			XMVectorSet(max.x, min.y, max.z, 1.0f),
			XMVectorSet(max.x, max.y, max.z, 1.0f)
	};

	// Transform the corners to world space
	for (size_t i = 0; i < 8; ++i)
	{
		corners[i] = XMVector3Transform(corners[i], modelMat);
	}

	// Check each frustum plane
	for (size_t i = 0; i < 6; ++i)
	{
		const XMVECTOR& plane = XMLoadFloat4(&_planes[i]);

		// Check if all corners are outside the plane
		bool allOutside = true;
		for (size_t j = 0; j < 8; ++j)
		{
			auto temp = XMVectorGetX(XMPlaneDotCoord(plane, corners[j]));
			if ( temp >= 0.0f)
			{
				// At least one corner is inside the plane
				allOutside = false;
				break;
			}
		}

		// If all corners are outside the plane, the AABB is not visible
		if (allOutside)
		{
			return false;
		}
	}

	// The AABB is inside all planes
	return true;
}