#include "Material.h"

Material::Material(D3D12_GPU_DESCRIPTOR_HANDLE textureBlockHandle)
{
	_gpuHandle = textureBlockHandle;
}

void Material::Bind(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, UINT rootParameterIndex)
{
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, _gpuHandle);
}