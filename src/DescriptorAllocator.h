#pragma once

#include "pch.h"
#include "D3D12Core.h"

namespace DescriptorAllocator
{
	namespace Resource
	{
		void InitializeDescriptorAllocator(uint32_t numDescriptors);

		extern D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
		extern D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
		extern ID3D12DescriptorHeap* GetHeap();
		
		extern MSWRL::ComPtr<ID3D12DescriptorHeap> heap;
		extern uint32_t descriptorSize;
		extern uint32_t capacity;
		extern std::atomic<uint32_t> currentOffset;
	}

	namespace RenderTarget
	{
		void InitializeDescriptorAllocator(uint32_t numDescriptors);

		extern D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
		extern D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
		extern ID3D12DescriptorHeap* GetHeap();

		extern MSWRL::ComPtr<ID3D12DescriptorHeap> heap;
		extern uint32_t descriptorSize;
		extern uint32_t capacity;
		extern std::atomic<uint32_t> currentOffset;
	}

	namespace Sampler
	{
		void InitializeDescriptorAllocator(uint32_t numDescriptors);

		extern D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
		extern D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);
		extern ID3D12DescriptorHeap* GetHeap();
		
		extern MSWRL::ComPtr<ID3D12DescriptorHeap> heap;
		extern uint32_t descriptorSize;
		extern uint32_t capacity;
		extern std::atomic<uint32_t> currentOffset;
	}
}