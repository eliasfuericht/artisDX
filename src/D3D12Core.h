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

		extern MSWRL::ComPtr<IDXGIFactory4> factory;
		extern MSWRL::ComPtr<IDXGIAdapter1> adapter;
		extern MSWRL::ComPtr<ID3D12Device> device;

#if defined(_DEBUG)
		extern MSWRL::ComPtr<ID3D12Debug1> debugController;
		extern MSWRL::ComPtr<ID3D12DebugDevice> debugDevice;
#endif
	}

	namespace Swapchain
	{
		void InitializeSwapchain(INT width, INT height, HWND window);
		void CreateDepthBuffer(INT newWidth, INT newHeight);
		void Resize(INT newWidth, INT newHeight);

		extern UINT width;
		extern UINT height;
		extern BOOL windowResized;

		extern MSWRL::ComPtr<IDXGISwapChain3> swapchain;
		extern D3D12_VIEWPORT viewport;
		extern D3D12_RECT surfaceSize;

		extern UINT frameIndex;
		extern UINT currentBuffer;
		static const UINT backBufferCount = 2;

		extern MSWRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
		extern UINT rtvDescriptorSize;
		extern D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle[backBufferCount];
		extern MSWRL::ComPtr<ID3D12Resource> renderTargets[backBufferCount];

		extern MSWRL::ComPtr<ID3D12Resource> depthStencilBuffer;
		extern MSWRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;
	}

	namespace ShaderCompiler
	{
		void InitializeShaderCompiler();

		extern MSWRL::ComPtr<IDxcUtils> utils;
		extern MSWRL::ComPtr<IDxcCompiler3> compiler;
		extern MSWRL::ComPtr<IDxcIncludeHandler> includeHandler;
	}
};