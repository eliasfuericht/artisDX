#include "MeshInstance.h"

MeshInstance::MeshInstance(INT MeshInstanceId, Mesh meshInstance, AABB aabbInstance, XMFLOAT4X4 localTransformMatrix, INT materialIndexInstance)
{
	_id = MeshInstanceId;
	_mesh = meshInstance;
	_aabb = aabbInstance;
	_localTransform = localTransformMatrix;
	_materialIndex = materialIndexInstance;

	CreateCBV();
}

void MeshInstance::CreateCBV()
{
	D3D12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle = DescriptorAllocator::Instance().Allocate();

	const UINT bufferSize = (sizeof(XMFLOAT4X4) + 255) & ~255;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	D3D12Core::GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_constantBuffer));

	CD3DX12_RANGE readRange(0, 0);
	_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&_mappedPtr));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	D3D12Core::GetDevice()->CreateConstantBufferView(&cbvDesc, cbvCpuHandle);

	_cbvGpuHandle = DescriptorAllocator::Instance().GetGPUHandle(cbvCpuHandle);
}
