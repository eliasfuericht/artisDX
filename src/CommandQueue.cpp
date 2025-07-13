#include "CommandQueue.h"

MSWRL::ComPtr<ID3D12CommandQueue> D3D12Core::CommandQueue::_commandQueue = nullptr;
MSWRL::ComPtr<ID3D12Fence> D3D12Core::CommandQueue::_fence = nullptr;
UINT64 D3D12Core::CommandQueue::_fenceValue = 0;
HANDLE D3D12Core::CommandQueue::_fenceEvent = nullptr;

void D3D12Core::CommandQueue::InitializeCommandQueue(D3D12_COMMAND_QUEUE_DESC queueDesc)
{
	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)), "CommandQueue creation failed!");

	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));

	_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
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