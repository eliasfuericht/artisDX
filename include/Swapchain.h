#pragma once

#include "pch.h"

#include "GraphicsDevice.h"
#include "CommandQueue.h"

namespace D3D12Core
{
	class Swapchain
	{
	public:
		static void Init(INT width, INT height, HWND window);
		static void CreateDepthBuffer(INT newWidth, INT newHeight);

		static void Resize(INT newWidth, INT newHeight);

		static UINT _width;
		static UINT _height;

		static MSWRL::ComPtr<IDXGISwapChain3> _swapchain;
		static D3D12_VIEWPORT _viewport;
		static D3D12_RECT _surfaceSize;

		static UINT _frameIndex;
		static UINT _currentBuffer;
		static const UINT _backBufferCount = 2;
		
		static MSWRL::ComPtr<ID3D12DescriptorHeap> _rtvHeap;
		static UINT _rtvDescriptorSize;
		static D3D12_CPU_DESCRIPTOR_HANDLE _rtvCPUHandle[_backBufferCount];
		static MSWRL::ComPtr<ID3D12Resource> _renderTargets[_backBufferCount];

		// DepthBuffer
		static MSWRL::ComPtr<ID3D12Resource> _depthStencilBuffer;
		static MSWRL::ComPtr<ID3D12DescriptorHeap> _dsvHeap;

		static BOOL _windowResized;
	};
}