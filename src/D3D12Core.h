#pragma once

#include "pch.h"

#include "CommandQueue.h"

namespace D3D12Core
{
	namespace GraphicsDevice
	{
		void InitializeFactory();
		void InitializeAdapter();
		void InitializeDevice();

#if defined(_DEBUG)
		void InitializeDebugController();
		void IntializeDebugDevice();
#endif

		extern MSWRL::ComPtr<IDXGIFactory4> _factory;
		extern MSWRL::ComPtr<IDXGIAdapter1> _adapter;
		extern MSWRL::ComPtr<ID3D12Device> _device;

#if defined(_DEBUG)
		extern MSWRL::ComPtr<ID3D12Debug1> _debugController;
		extern MSWRL::ComPtr<ID3D12DebugDevice> _debugDevice;
#endif
	}

	namespace Swapchain
	{
		void InitializeSwapchain(INT width, INT height, HWND window);
		void CreateDepthBuffer(INT newWidth, INT newHeight);
		void Resize(INT newWidth, INT newHeight);

		extern UINT _width;
		extern UINT _height;
		extern BOOL _windowResized;

		extern MSWRL::ComPtr<IDXGISwapChain3> _swapchain;
		extern D3D12_VIEWPORT _viewport;
		extern D3D12_RECT _surfaceSize;

		extern UINT _frameIndex;
		extern UINT _currentBuffer;
		static const UINT _backBufferCount = 2;

		extern MSWRL::ComPtr<ID3D12DescriptorHeap> _rtvHeap;
		extern UINT _rtvDescriptorSize;
		extern D3D12_CPU_DESCRIPTOR_HANDLE _rtvCPUHandle[_backBufferCount];
		extern MSWRL::ComPtr<ID3D12Resource> _renderTargets[_backBufferCount];

		extern MSWRL::ComPtr<ID3D12Resource> _depthStencilBuffer;
		extern MSWRL::ComPtr<ID3D12DescriptorHeap> _dsvHeap;
	}

	namespace ShaderCompiler
	{
		void InitializeShaderCompiler();

		extern MSWRL::ComPtr<IDxcUtils> _utils;
		extern MSWRL::ComPtr<IDxcCompiler3> _compiler;
		extern MSWRL::ComPtr<IDxcIncludeHandler> _includeHandler;
	}
};