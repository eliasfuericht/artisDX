#pragma once

#include "precompiled/pch.h"

#include "GraphicsDevice.h"
#include "CommandQueue.h"
#include "Window.h"

namespace D3D12Core
{
	class Swapchain
	{
	public:
		static void Init(Window& window);

		static UINT _width;
		static UINT _height;
		static MSWRL::ComPtr<IDXGISwapChain3> _swapchain;
		static D3D12_VIEWPORT _viewport;
		static D3D12_RECT _surfaceSize;
		static UINT _currentBuffer;
		static const UINT _backBufferCount = 2;
		static MSWRL::ComPtr<ID3D12DescriptorHeap> _rtvHeap;
		static UINT _rtvDescriptorSize;
		static D3D12_CPU_DESCRIPTOR_HANDLE _rtvDescriptor[_backBufferCount];
		static MSWRL::ComPtr<ID3D12Resource> _renderTargets[_backBufferCount];
		static UINT _frameIndex;
	};
}