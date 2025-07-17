#pragma once

#include "pch.h"
#include "D3D12Core.h"

enum QUEUETYPE : int32_t
{
	QUEUE_INVALID = NOTOK,
	QUEUE_GRAPHICS = 0,
	QUEUE_COMPUTE = 1,
	QUEUE_UPLOAD = 2
};

class CommandQueue
{
public:
	void InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE queuetype);

	void WaitForFence();

	MSWRL::ComPtr<ID3D12CommandQueue> _commandQueue;
	MSWRL::ComPtr<ID3D12Fence> _fence;

	uint64_t _fenceValue;
	HANDLE _fenceEvent;
};

namespace CommandQueueManager
{
	extern void InitializeCommandQueueManager();
	
	extern CommandQueue& GetCommandQueue(QUEUETYPE queueType);
	
	extern CommandQueue commandQueues[3];
}