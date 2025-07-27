#include "DescriptorAllocator.h"

namespace DescriptorAllocator
{
	namespace CBVSRVUAV
	{
		MSWRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
		uint32_t descriptorSize = 0;
		uint32_t capacity = 0;
		std::atomic<uint32_t> currentOffset = 0;

		void InitializeDescriptorAllocator(uint32_t numDescriptors)
		{
			DescriptorAllocator::CBVSRVUAV::capacity = numDescriptors;
			DescriptorAllocator::CBVSRVUAV::descriptorSize = D3D12Core::GraphicsDevice::device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = numDescriptors;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&DescriptorAllocator::CBVSRVUAV::heap)));
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			uint32_t offset = DescriptorAllocator::CBVSRVUAV::currentOffset++;
			if (offset >= DescriptorAllocator::CBVSRVUAV::capacity)
			{
				throw std::runtime_error("CBV/SRV/UAV heap out of space!");
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = DescriptorAllocator::CBVSRVUAV::heap->GetCPUDescriptorHandleForHeapStart();
			cpuHandle.ptr += offset * DescriptorAllocator::CBVSRVUAV::descriptorSize;
			return cpuHandle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = DescriptorAllocator::CBVSRVUAV::heap->GetGPUDescriptorHandleForHeapStart();
			uint32_t offset = static_cast<uint32_t>((cpuHandle.ptr - DescriptorAllocator::CBVSRVUAV::heap->GetCPUDescriptorHandleForHeapStart().ptr) / DescriptorAllocator::CBVSRVUAV::descriptorSize);
			gpuHandle.ptr += offset * DescriptorAllocator::CBVSRVUAV::descriptorSize;
			return gpuHandle;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return DescriptorAllocator::CBVSRVUAV::heap.Get();
		}
	}

	namespace RTV
	{
		MSWRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
		uint32_t descriptorSize = 0;
		uint32_t capacity = 0;
		std::atomic<uint32_t> currentOffset = 0;

		void InitializeDescriptorAllocator(uint32_t numDescriptors)
		{
			DescriptorAllocator::RTV::capacity = numDescriptors;
			DescriptorAllocator::RTV::descriptorSize = D3D12Core::GraphicsDevice::device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = numDescriptors;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&DescriptorAllocator::RTV::heap)));
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			uint32_t offset = DescriptorAllocator::RTV::currentOffset++;
			if (offset >= DescriptorAllocator::RTV::capacity)
			{
				throw std::runtime_error("RTV heap out of space!");
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = DescriptorAllocator::RTV::heap->GetCPUDescriptorHandleForHeapStart();
			cpuHandle.ptr += offset * DescriptorAllocator::RTV::descriptorSize;
			return cpuHandle;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return DescriptorAllocator::RTV::heap.Get();
		}
	}

	namespace DSV
	{
		MSWRL::ComPtr<ID3D12DescriptorHeap> heap = nullptr;
		uint32_t descriptorSize = 0;
		uint32_t capacity = 0;
		std::atomic<uint32_t> currentOffset = 0;

		void InitializeDescriptorAllocator(uint32_t numDescriptors)
		{
			DescriptorAllocator::DSV::capacity = numDescriptors;
			DescriptorAllocator::DSV::descriptorSize = D3D12Core::GraphicsDevice::device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = numDescriptors;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&DescriptorAllocator::DSV::heap)));
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			uint32_t offset = DescriptorAllocator::DSV::currentOffset++;
			if (offset >= DescriptorAllocator::DSV::capacity)
			{
				throw std::runtime_error("DSV heap out of space!");
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = DescriptorAllocator::DSV::heap->GetCPUDescriptorHandleForHeapStart();
			cpuHandle.ptr += offset * DescriptorAllocator::DSV::descriptorSize;
			return cpuHandle;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return DescriptorAllocator::DSV::heap.Get();
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