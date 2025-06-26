#pragma once

#include "precompiled/pch.h"

class DescriptorAllocator
{
public:
	static DescriptorAllocator& Instance();

	void Initialize(ID3D12Device* device, UINT numDescriptors);

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) const;
	ID3D12DescriptorHeap* GetHeap() const;

private:
	DescriptorAllocator() = default;

	MSWRL::ComPtr<ID3D12DescriptorHeap> _heap;
	UINT _descriptorSize = 0;
	UINT _capacity = 0;
	std::atomic<UINT> _currentOffset = 0;
	std::mutex _allocationMutex;
};