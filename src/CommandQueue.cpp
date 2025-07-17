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
	const uint64_t fence = _fenceValue;
	if (_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(_fence->SetEventOnCompletion(fence, _fenceEvent));
		WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
	}
}

namespace CommandQueueManager
{
	CommandQueue commandQueues[3];

	void InitializeCommandQueueManager()
	{
		CommandQueueManager::commandQueues[QUEUETYPE::QUEUE_GRAPHICS].InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
		CommandQueueManager::commandQueues[QUEUETYPE::QUEUE_COMPUTE].InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
		CommandQueueManager::commandQueues[QUEUETYPE::QUEUE_UPLOAD].InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	}

	CommandQueue& GetCommandQueue(QUEUETYPE queueType)
	{
		return CommandQueueManager::commandQueues[queueType];
	}
}
