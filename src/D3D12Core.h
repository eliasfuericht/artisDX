#pragma once

#include "pch.h"

#include "CommandQueue.h"
#include "Window.h"

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
		void InitializeSwapchain();
		void CreateDepthBuffer(int32_t newWidth, int32_t newHeight);
		void Resize(int32_t newWidth, int32_t newHeight);

		extern uint32_t width;
		extern uint32_t height;
		extern bool windowResized;

		extern MSWRL::ComPtr<IDXGISwapChain3> swapchain;
		extern D3D12_VIEWPORT viewport;
		extern D3D12_RECT surfaceSize;

		extern uint32_t frameIndex;
		extern uint32_t currentBuffer;
		static const uint32_t backBufferCount = 2;

		extern MSWRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
		extern uint32_t rtvDescriptorSize;
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