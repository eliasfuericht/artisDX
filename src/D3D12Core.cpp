#include "D3D12Core.h"

// ----------- Device
MSWRL::ComPtr<ID3D12Device> D3D12Core::Device::_device = nullptr;

void D3D12Core::Device::Initialize(MSWRL::ComPtr<ID3D12Device> device)
{
	_device = device;
}

MSWRL::ComPtr<ID3D12Device> D3D12Core::Device::Get()
{
	return _device;
}