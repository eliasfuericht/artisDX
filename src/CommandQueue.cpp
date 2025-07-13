#include "CommandQueue.h"

MSWRL::ComPtr<ID3D12CommandQueue> D3D12Core::CommandQueue::_commandQueue = nullptr;
MSWRL::ComPtr<ID3D12Fence> D3D12Core::CommandQueue::_fence = nullptr;
UINT64 D3D12Core::CommandQueue::_fenceValue = 0;
HANDLE D3D12Core::CommandQueue::_fenceEvent = nullptr;

void D3D12Core::CommandQueue::InitializeCommandQueue(MSWRL::ComPtr<ID3D12CommandQueue> commandQueue)
{
	_commandQueue = commandQueue;
}

void D3D12Core::CommandQueue::InitializeFence(MSWRL::ComPtr<ID3D12Fence> fence)
{
	_fence = fence;

	_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

MSWRL::ComPtr<ID3D12CommandQueue> D3D12Core::CommandQueue::GetCommandQueue()
{
	return _commandQueue;
}

void D3D12Core::CommandQueue::WaitForFence()
{
	_fenceValue++;
	_commandQueue->Signal(_fence.Get(), _fenceValue);
	const UINT64 fence = _fenceValue;
	if (_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(_fence->SetEventOnCompletion(fence, _fenceEvent));
		WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
	}
}