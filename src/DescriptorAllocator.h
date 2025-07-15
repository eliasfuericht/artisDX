#pragma once

#include "pch.h"
#include "D3D12Core.h"

namespace DescriptorAllocator
{
	namespace Resource
	{
		void InitializeDescriptorAllocator(UINT numDescriptors);

		extern D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
		extern D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
		extern ID3D12DescriptorHeap* GetHeap();
		
		extern MSWRL::ComPtr<ID3D12DescriptorHeap> _heap;
		extern UINT _descriptorSize;
		extern UINT _capacity;
		extern std::atomic<UINT> _currentOffset;
	}

	namespace Sampler
	{
		void InitializeDescriptorAllocator(UINT numDescriptors);

		extern D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
		extern D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
		extern ID3D12DescriptorHeap* GetHeap();
		
		extern MSWRL::ComPtr<ID3D12DescriptorHeap> _heap;
		extern UINT _descriptorSize;
		extern UINT _capacity;
		extern std::atomic<UINT> _currentOffset;
	}
}