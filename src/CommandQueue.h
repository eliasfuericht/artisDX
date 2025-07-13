#pragma once

#include "pch.h"
#include "GraphicsDevice.h"

namespace D3D12Core
{
	class CommandQueue
	{
	public:
		static void InitializeCommandQueue(D3D12_COMMAND_QUEUE_DESC queueDesc);

		static void WaitForFence();

		static MSWRL::ComPtr<ID3D12CommandQueue> _commandQueue;
		static MSWRL::ComPtr<ID3D12Fence> _fence;

		static UINT64 _fenceValue;
		static HANDLE _fenceEvent;
	};
}