#include "MeshInstance.h"

MeshInstance::MeshInstance(INT MeshInstanceId, Mesh meshInstance, AABB aabbInstance, XMFLOAT4X4 localTransformMatrix, MSWRL::ComPtr<ID3D12Device> device)
{
	id = MeshInstanceId;
	mesh = meshInstance;
	aabb = aabbInstance;
	localTransform = localTransformMatrix;

	CreateCBV(device);
}

void MeshInstance::CreateCBV(MSWRL::ComPtr<ID3D12Device> device)
{
	D3D12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle = DescriptorAllocator::Instance().Allocate();

	const UINT bufferSize = (sizeof(XMFLOAT4X4) + 255) & ~255;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantBuffer));

	CD3DX12_RANGE readRange(0, 0);
	constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedPtr));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	device->CreateConstantBufferView(&cbvDesc, cbvCpuHandle);

	cbvGpuHandle = DescriptorAllocator::Instance().GetGPUHandle(cbvCpuHandle);
}
