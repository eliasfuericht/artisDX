#include "CommandQueue.h"

void CommandQueue::InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE queuetype)
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = queuetype;

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)), "CommandQueue creation failed!");

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));

	_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (_fenceEvent == nullptr)
	{
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void CommandQueue::WaitForFence()
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

namespace CommandQueueManager
{
	CommandQueue _commandQueues[3];

	void InitializeCommandQueueManager()
	{
		_commandQueues[(INT)QUEUETYPE::GRAPHICS].InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
		_commandQueues[(INT)QUEUETYPE::COMPUTE].InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
		_commandQueues[(INT)QUEUETYPE::UPLOAD].InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	}

	CommandQueue& GetCommandQueue(QUEUETYPE queueType)
	{
		return _commandQueues[(INT)queueType];
	}
}
