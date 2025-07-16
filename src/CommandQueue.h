#pragma once

#include "pch.h"
#include "D3D12Core.h"

class CommandQueue
{
public:
	void InitializeCommandQueue(D3D12_COMMAND_LIST_TYPE queuetype);

	void WaitForFence();

	MSWRL::ComPtr<ID3D12CommandQueue> _commandQueue;
	MSWRL::ComPtr<ID3D12Fence> _fence;

	UINT64 _fenceValue;
	HANDLE _fenceEvent;
};

namespace CommandQueueManager
{
	enum QUEUETYPE
	{
		GRAPHICS = 0,
		COMPUTE = 1,
		UPLOAD = 2
	};

	extern void InitializeCommandQueueManager();
	
	extern CommandQueue& GetCommandQueue(QUEUETYPE queueType);
	
	extern CommandQueue _commandQueues[3];
}