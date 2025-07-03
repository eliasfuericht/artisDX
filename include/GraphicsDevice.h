#pragma once

#include "pch.h"

namespace D3D12Core 
{
	class GraphicsDevice
	{
	public:
		static void InitializeFactory(MSWRL::ComPtr<IDXGIFactory4> factory);
		static void InitializeAdapter(MSWRL::ComPtr<IDXGIAdapter1> adapter);
		static void InitializeDevice(MSWRL::ComPtr<ID3D12Device> device);

		static MSWRL::ComPtr<IDXGIFactory4> GetFactory();
		static MSWRL::ComPtr<IDXGIAdapter1> GetAdapter();
		static MSWRL::ComPtr<ID3D12Device> GetDevice();

#if defined(_DEBUG)
		static void InitializeDebugController(MSWRL::ComPtr<ID3D12Debug1> debugController);
		static void IntializeDebugDevice(MSWRL::ComPtr<ID3D12DebugDevice> debugDevice);

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
};