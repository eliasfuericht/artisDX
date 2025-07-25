#include "DescriptorAllocator.h"

namespace DescriptorAllocator
{
	namespace Resource
	{
		MSWRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
		uint32_t descriptorSize = 0;
		uint32_t capacity = 0;
		std::atomic<uint32_t> currentOffset = 0;

		void InitializeDescriptorAllocator(uint32_t numDescriptors)
		{
			DescriptorAllocator::Resource::capacity = numDescriptors;
			DescriptorAllocator::Resource::descriptorSize = D3D12Core::GraphicsDevice::device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = numDescriptors;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&DescriptorAllocator::Resource::heap)));
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			uint32_t offset = DescriptorAllocator::Resource::currentOffset++;
			if (offset >= DescriptorAllocator::Resource::capacity)
			{
				throw std::runtime_error("CBV/SRV/UAV heap out of space!");
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = DescriptorAllocator::Resource::heap->GetCPUDescriptorHandleForHeapStart();
			cpuHandle.ptr += offset * DescriptorAllocator::Resource::descriptorSize;
			return cpuHandle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = DescriptorAllocator::Resource::heap->GetGPUDescriptorHandleForHeapStart();
			uint32_t offset = static_cast<uint32_t>((cpuHandle.ptr - DescriptorAllocator::Resource::heap->GetCPUDescriptorHandleForHeapStart().ptr) / DescriptorAllocator::Resource::descriptorSize);
			gpuHandle.ptr += offset * DescriptorAllocator::Resource::descriptorSize;
			return gpuHandle;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return DescriptorAllocator::Resource::heap.Get();
		}
	}

	namespace RenderTarget
	{
		MSWRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
		uint32_t descriptorSize = 0;
		uint32_t capacity = 0;
		std::atomic<uint32_t> currentOffset = 0;

		void InitializeDescriptorAllocator(uint32_t numDescriptors)
		{
			DescriptorAllocator::RenderTarget::capacity = numDescriptors;
			DescriptorAllocator::RenderTarget::descriptorSize = D3D12Core::GraphicsDevice::device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = numDescriptors;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&DescriptorAllocator::RenderTarget::heap)));
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			uint32_t offset = DescriptorAllocator::RenderTarget::currentOffset++;
			if (offset >= DescriptorAllocator::RenderTarget::capacity)
			{
				throw std::runtime_error("RTV heap out of space!");
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = DescriptorAllocator::RenderTarget::heap->GetCPUDescriptorHandleForHeapStart();
			cpuHandle.ptr += offset * DescriptorAllocator::RenderTarget::descriptorSize;
			return cpuHandle;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return DescriptorAllocator::RenderTarget::heap.Get();
		}
	}

	namespace DepthStencil
	{
		MSWRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
		uint32_t descriptorSize = 0;
		uint32_t capacity = 0;
		std::atomic<uint32_t> currentOffset = 0;

		void InitializeDescriptorAllocator(uint32_t numDescriptors)
		{
			DescriptorAllocator::DepthStencil::capacity = numDescriptors;
			DescriptorAllocator::DepthStencil::descriptorSize = D3D12Core::GraphicsDevice::device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = numDescriptors;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&DescriptorAllocator::DepthStencil::heap)));
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			uint32_t offset = DescriptorAllocator::DepthStencil::currentOffset++;
			if (offset >= DescriptorAllocator::DepthStencil::capacity)
			{
				throw std::runtime_error("DSV heap out of space!");
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = DescriptorAllocator::DepthStencil::heap->GetCPUDescriptorHandleForHeapStart();
			cpuHandle.ptr += offset * DescriptorAllocator::DepthStencil::descriptorSize;
			return cpuHandle;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return DescriptorAllocator::DepthStencil::heap.Get();
		}
	}

	namespace Sampler
	{
		MSWRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
		uint32_t descriptorSize = 0;
		uint32_t capacity = 0;
		std::atomic<uint32_t> currentOffset = 0;

		void InitializeDescriptorAllocator(uint32_t numDescriptors)
		{
			DescriptorAllocator::Sampler::capacity = numDescriptors;
			DescriptorAllocator::Sampler::descriptorSize = D3D12Core::GraphicsDevice::device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = numDescriptors;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&DescriptorAllocator::Sampler::heap)));
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			uint32_t offset = DescriptorAllocator::Sampler::currentOffset++;
			if (offset >= DescriptorAllocator::Sampler::capacity)
			{
				throw std::runtime_error("Sampler heap out of space!");
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = DescriptorAllocator::Sampler::heap->GetCPUDescriptorHandleForHeapStart();
			cpuHandle.ptr += offset * DescriptorAllocator::Sampler::descriptorSize;
			return cpuHandle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = DescriptorAllocator::Sampler::heap->GetGPUDescriptorHandleForHeapStart();
			uint32_t offset = static_cast<uint32_t>((cpuHandle.ptr - DescriptorAllocator::Sampler::heap->GetCPUDescriptorHandleForHeapStart().ptr) / DescriptorAllocator::Sampler::descriptorSize);
			gpuHandle.ptr += offset * DescriptorAllocator::Sampler::descriptorSize;
			return gpuHandle;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return DescriptorAllocator::Sampler::heap.Get();
		}
	}
}