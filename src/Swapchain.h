#pragma once

#include "pch.h"

#include "GraphicsDevice.h"
#include "CommandQueue.h"

namespace D3D12Core::Swapchain
{
	void InitializeSwapchain(INT width, INT height, HWND window);
	void CreateDepthBuffer(INT newWidth, INT newHeight);

	void Resize(INT newWidth, INT newHeight);

	extern UINT _width;
	extern UINT _height;
	
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

	// DepthBuffer
	extern MSWRL::ComPtr<ID3D12Resource> _depthStencilBuffer;
	extern MSWRL::ComPtr<ID3D12DescriptorHeap> _dsvHeap;
	
	extern BOOL _windowResized;
}