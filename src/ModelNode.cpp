#include "ModelNode.h"

ModelNode::ModelNode()
{
	CreateCBV();
}

void ModelNode::CreateCBV()
{
	D3D12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle = DescriptorAllocator::Resource::Allocate();

	const uint32_t bufferSize = (sizeof(XMFLOAT4X4) + 255) & ~255;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_ModelMatrixBufferResource));

	CD3DX12_RANGE readRange(0, 0);
	_ModelMatrixBufferResource->Map(0, &readRange, reinterpret_cast<void**>(&_mappedCBVModelMatrixPtr));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _ModelMatrixBufferResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	D3D12Core::GraphicsDevice::device->CreateConstantBufferView(&cbvDesc, cbvCpuHandle);

	_cbvModelMatrixGpuHandle = DescriptorAllocator::Resource::GetGPUHandle(cbvCpuHandle);
}