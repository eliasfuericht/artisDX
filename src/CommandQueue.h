#pragma once

#include "pch.h"

namespace D3D12Core
{
	class CommandQueue
	{
	public:
		static void InitializeCommandQueue(MSWRL::ComPtr<ID3D12CommandQueue> commandQueue);
		static void InitializeFence(MSWRL::ComPtr<ID3D12Fence> fence);

		static MSWRL::ComPtr<ID3D12CommandQueue> GetCommandQueue();

		static void WaitForFence();

	private:
		static MSWRL::ComPtr<ID3D12CommandQueue> _commandQueue;
		static MSWRL::ComPtr<ID3D12Fence> _fence;

		static UINT64 _fenceValue;
		static HANDLE _fenceEvent;
	};
}