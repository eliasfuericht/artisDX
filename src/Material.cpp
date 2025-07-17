#include "Material.h"

Material::Material()
{
	CreateCBV();
}

void Material::CreateCBV()
{
	D3D12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle = DescriptorAllocator::Resource::Allocate();

	const uint32_t bufferSize = (sizeof(PBRFactors) + 255) & ~255;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_MaterialFactorsBufferResource));

	CD3DX12_RANGE readRange(0, 0);
	_MaterialFactorsBufferResource->Map(0, &readRange, reinterpret_cast<void**>(&_mappedMaterialFactorsPtr));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _MaterialFactorsBufferResource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	D3D12Core::GraphicsDevice::device->CreateConstantBufferView(&cbvDesc, cbvCpuHandle);

	_cbvMaterialFactorsGpuHandle = DescriptorAllocator::Resource::GetGPUHandle(cbvCpuHandle);
}

void Material::BindMaterialFactorsData(const ShaderPass& shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	memcpy(_mappedMaterialFactorsPtr, &_pbrFactors, sizeof(PBRFactors));

	if (auto slot = shaderPass.GetRootParameterIndex("pbrFactors"))
		commandList->SetGraphicsRootDescriptorTable(*slot, _cbvMaterialFactorsGpuHandle);
}
