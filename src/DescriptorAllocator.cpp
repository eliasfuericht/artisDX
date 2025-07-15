#include "DescriptorAllocator.h"

MSWRL::ComPtr<ID3D12DescriptorHeap> DescriptorAllocator::_heap = nullptr;
UINT DescriptorAllocator::_descriptorSize = 0;
UINT DescriptorAllocator::_capacity = 0;
std::atomic<UINT> DescriptorAllocator::_currentOffset = 0;

void DescriptorAllocator::InitializeDescriptorAllocator(UINT numDescriptors)
{
	_capacity = numDescriptors;
	_descriptorSize = D3D12Core::GraphicsDevice::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = numDescriptors;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_heap)));
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocate()
{
	// std::lock_guard<std::mutex> lock(_allocationMutex);

	UINT offset = _currentOffset++;
	if (offset >= _capacity)
	{
		throw std::runtime_error("Descriptor heap out of space");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = _heap->GetCPUDescriptorHandleForHeapStart();
	cpuHandle.ptr += offset * _descriptorSize;
	return cpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
{
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = _heap->GetGPUDescriptorHandleForHeapStart();
	UINT offset = static_cast<UINT>((cpuHandle.ptr - _heap->GetCPUDescriptorHandleForHeapStart().ptr) / _descriptorSize);
	gpuHandle.ptr += offset * _descriptorSize;
	return gpuHandle;
}

ID3D12DescriptorHeap* DescriptorAllocator::GetHeap()
{
	return _heap.Get();
}