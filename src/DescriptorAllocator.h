#pragma once

#include "pch.h"
#include "D3D12Core.h"

class DescriptorAllocator
{
public:
	static void InitializeDescriptorAllocator(UINT numDescriptors);

	static D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
	static ID3D12DescriptorHeap* GetHeap();

	static MSWRL::ComPtr<ID3D12DescriptorHeap> _heap;
	static UINT _descriptorSize;
	static UINT _capacity;
	static std::atomic<UINT> _currentOffset;
};