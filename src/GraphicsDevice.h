#pragma once

#include "pch.h"

namespace D3D12Core::GraphicsDevice
{
	void InitializeFactory(UINT flags);
	void InitializeAdapter();
	void InitializeDevice();

	MSWRL::ComPtr<IDXGIFactory4> GetFactory();
	MSWRL::ComPtr<IDXGIAdapter1> GetAdapter();
	MSWRL::ComPtr<ID3D12Device> GetDevice();

#if defined(_DEBUG)
	void InitializeDebugController();
	void IntializeDebugDevice();

	MSWRL::ComPtr<ID3D12Debug1> GetDebugController();
	MSWRL::ComPtr<ID3D12DebugDevice> GetDebugDevice();
#endif

	extern MSWRL::ComPtr<IDXGIFactory4> _factory;
	extern MSWRL::ComPtr<IDXGIAdapter1> _adapter;
	extern MSWRL::ComPtr<ID3D12Device> _device;

#if defined(_DEBUG)
	extern MSWRL::ComPtr<ID3D12Debug1> _debugController;
	extern MSWRL::ComPtr<ID3D12DebugDevice> _debugDevice;
#endif
};