#pragma once

#include "precompiled/pch.h"

namespace D3D12Core 
{
	class GraphicsDevice
	{
	public:
		static void InitializeFactory(MSWRL::ComPtr<IDXGIFactory4> factory);
		static void InitializeAdapter(MSWRL::ComPtr<IDXGIAdapter1> adapter);
		static void InitializeDevice(MSWRL::ComPtr<ID3D12Device> device);

#if defined(_DEBUG)
		static void InitializeDebugController(MSWRL::ComPtr<ID3D12Debug1> debugController);
		static void IntializeDebugDevice(MSWRL::ComPtr<ID3D12DebugDevice> debugDevice);
#endif

		static MSWRL::ComPtr<IDXGIFactory4> GetFactory();
		static MSWRL::ComPtr<IDXGIAdapter1> GetAdapter();
		static MSWRL::ComPtr<ID3D12Device> GetDevice();

#if defined(_DEBUG)
		static MSWRL::ComPtr<ID3D12Debug1> GetDebugController();
		static MSWRL::ComPtr<ID3D12DebugDevice> GetDebugDevice();
#endif

	private:
		static MSWRL::ComPtr<IDXGIFactory4> _factory;
		static MSWRL::ComPtr<IDXGIAdapter1> _adapter;
		static MSWRL::ComPtr<ID3D12Device> _device;

#if defined(_DEBUG)
		static MSWRL::ComPtr<ID3D12Debug1> _debugController;
		static MSWRL::ComPtr<ID3D12DebugDevice> _debugDevice;
#endif
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