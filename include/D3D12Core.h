#pragma once

#include "precompiled/pch.h"

namespace D3D12Core 
{
	class Device 
	{
	public:
		static void Initialize(MSWRL::ComPtr<ID3D12Device> device);
		static MSWRL::ComPtr<ID3D12Device> Get();
	private:
		static MSWRL::ComPtr<ID3D12Device> _device;
	};

	class CommandQueue {
	public:
		static void Initialize(D3D12_COMMAND_LIST_TYPE type);
		static CommandQueue& Get();

		void ExecuteCommandList(MSWRL::ComPtr<ID3D12GraphicsCommandList> cmdList);
		void Flush();

		MSWRL::ComPtr<ID3D12CommandQueue> GetQueue() const { return _queue; }

	private:
		CommandQueue() = default;

		static CommandQueue _instance;

		MSWRL::ComPtr<ID3D12CommandQueue> _queue;
		MSWRL::ComPtr<ID3D12Fence> _fence;
		HANDLE _fenceEvent = nullptr;
		UINT64 _fenceValue = 0;
	};
};