#include "D3D12Core.h"

MSWRL::ComPtr<ID3D12Device> D3D12Core::_device = nullptr;

void D3D12Core::Initialize(MSWRL::ComPtr<ID3D12Device> device)
{
	_device = device;
}

MSWRL::ComPtr<ID3D12Device> D3D12Core::GetDevice()
{
	return _device;
}