#pragma once

#include "precompiled/pch.h"

// maybe extend to swapchain, commandqueue etc after i know more about DX12
class D3D12Core {
public:
	static void Initialize(MSWRL::ComPtr<ID3D12Device> device);

	static MSWRL::ComPtr<ID3D12Device> GetDevice();

private:
	static MSWRL::ComPtr<ID3D12Device> _device;
};