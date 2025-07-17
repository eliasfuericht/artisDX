#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(float x, float y, float z)
{
	_direction = XMFLOAT3(x, y, z);
	CreateCBV();
}

void DirectionalLight::CreateCBV()
{
	_cbvdLightCPUHandle = DescriptorAllocator::Resource::Allocate();

	const uint32_t bufferSize = (sizeof(XMFLOAT3) + 255) & ~255;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_dLightBufferResource));

	CD3DX12_RANGE readRange(0, 0);
	_dLightBufferResource->Map(0, &readRange, reinterpret_cast<void**>(&_mappedCBVdLightPtr));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _dLightBufferResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	D3D12Core::GraphicsDevice::device->CreateConstantBufferView(&cbvDesc, _cbvdLightCPUHandle);
}

void DirectionalLight::UpdateBuffer()
{
	memcpy(_mappedCBVdLightPtr, &_direction, sizeof(XMFLOAT3));
}

void DirectionalLight::DrawGUI()
{
	std::string windowName = "DirectionalLight";
	GUI::Begin(windowName.c_str());

	GUI::DragFloat3("Direction", _direction);

	GUI::End();
}