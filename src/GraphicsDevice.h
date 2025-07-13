#pragma once

#include "pch.h"

namespace D3D12Core 
{
	class GraphicsDevice
	{
	public:
		static void InitializeFactory(UINT flags);
		static void InitializeAdapter();
		static void InitializeDevice();

		static MSWRL::ComPtr<IDXGIFactory4> GetFactory();
		static MSWRL::ComPtr<IDXGIAdapter1> GetAdapter();
		static MSWRL::ComPtr<ID3D12Device> GetDevice();

#if defined(_DEBUG)
		static void InitializeDebugController();
		static void IntializeDebugDevice();

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