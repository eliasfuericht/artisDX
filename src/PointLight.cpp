#include "PointLight.h"

PointLight::PointLight(float x, float y, float z)
{
	_position = XMFLOAT3(x, y, z);
	CreateCBV();
}

void PointLight::CreateCBV()
{
	_cbvpLightCPUHandle = DescriptorAllocator::Resource::Allocate();

	const UINT bufferSize = (sizeof(XMFLOAT3) + 255) & ~255;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	D3D12Core::GraphicsDevice::_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_pLightBufferResource));

	CD3DX12_RANGE readRange(0, 0);
	_pLightBufferResource->Map(0, &readRange, reinterpret_cast<void**>(&_mappedCBVpLightPtr));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _pLightBufferResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	D3D12Core::GraphicsDevice::_device->CreateConstantBufferView(&cbvDesc, _cbvpLightCPUHandle);
}

void PointLight::UpdateBuffer()
{
	memcpy(_mappedCBVpLightPtr, &_position, sizeof(XMFLOAT3));
}

void PointLight::DrawGUI() 
{
	std::string windowName = "dLight";
	GUI::Begin(windowName.c_str());
	
	GUI::DragFloat3("light direction", _position);

	GUI::End();
}