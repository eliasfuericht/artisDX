#include "DescriptorAllocator.h"

namespace DescriptorAllocator
{
	namespace Resource
	{
		MSWRL::ComPtr<ID3D12DescriptorHeap> _heap = nullptr;
		UINT _descriptorSize = 0;
		UINT _capacity = 0;
		std::atomic<UINT> _currentOffset = 0;

		void InitializeDescriptorAllocator(UINT numDescriptors)
		{
			_capacity = numDescriptors;
			_descriptorSize = D3D12Core::GraphicsDevice::_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = numDescriptors;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap)));
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			UINT offset = _currentOffset++;
			if (offset >= _capacity)
			{
				throw std::runtime_error("CBV/SRV/UAV heap out of space!");
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _heap->GetCPUDescriptorHandleForHeapStart();
			cpuHandle.ptr += offset * _descriptorSize;
			return cpuHandle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = _heap->GetGPUDescriptorHandleForHeapStart();
			UINT offset = static_cast<UINT>((cpuHandle.ptr - _heap->GetCPUDescriptorHandleForHeapStart().ptr) / _descriptorSize);
			gpuHandle.ptr += offset * _descriptorSize;
			return gpuHandle;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return _heap.Get();
		}
	}

	namespace Sampler
	{
		MSWRL::ComPtr<ID3D12DescriptorHeap> _heap = nullptr;
		UINT _descriptorSize = 0;
		UINT _capacity = 0;
		std::atomic<UINT> _currentOffset = 0;

		void InitializeDescriptorAllocator(UINT numDescriptors)
		{
			_capacity = numDescriptors;
			_descriptorSize = D3D12Core::GraphicsDevice::_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = numDescriptors;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap)));
		}

		D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
		{
			UINT offset = _currentOffset++;
			if (offset >= _capacity)
			{
				throw std::runtime_error("Sampler heap out of space!");
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _heap->GetCPUDescriptorHandleForHeapStart();
			cpuHandle.ptr += offset * _descriptorSize;
			return cpuHandle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
		{
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = _heap->GetGPUDescriptorHandleForHeapStart();
			UINT offset = static_cast<UINT>((cpuHandle.ptr - _heap->GetCPUDescriptorHandleForHeapStart().ptr) / _descriptorSize);
			gpuHandle.ptr += offset * _descriptorSize;
			return gpuHandle;
		}

		ID3D12DescriptorHeap* GetHeap()
		{
			return _heap.Get();
		}
	}
}