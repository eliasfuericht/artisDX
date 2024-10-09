#pragma once

#include "pch.h"
#include "Window.h"

class Renderer
{
	static const UINT backBufferCount = 2;

public:
	Renderer(std::weak_ptr<Window> window);

private:
	CHECK InitializeAPI();
	CHECK CreateSwapchain(UINT w, UINT h);
	CHECK DestroyFrameBuffer();
	CHECK SetupSwapchain();

	std::weak_ptr<Window> _window;
	UINT _width;
	UINT _height;

	// Initialization
	IDXGIFactory4* _factory;
	IDXGIAdapter1* _adapter;
#if defined(_DEBUG)
	ID3D12Debug1* _debugController;
	ID3D12DebugDevice* _debugDevice;
#endif
	ID3D12Device* _device;
	ID3D12CommandQueue* _commandQueue;
	ID3D12CommandAllocator* _commandAllocator;
	ID3D12GraphicsCommandList* _commandList;

	UINT _currentBuffer;
	ID3D12DescriptorHeap* _rtvHeap;
	ID3D12Resource* _renderTargets[backBufferCount];
	IDXGISwapChain3* _swapchain;

	// Resources
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _surfaceSize;

	ID3D12Resource* _vertexBuffer;
	ID3D12Resource* _indexBuffer;

	ID3D12Resource* _uniformBuffer;
	ID3D12DescriptorHeap* _uniformBufferHeap;
	UINT8* _mappedUniformBuffer;

	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;

	UINT _rtvDescriptorSize;
	ID3D12RootSignature* _rootSignature;
	ID3D12PipelineState* _pipelineState;

	// Sync
	UINT _frameIndex;
	HANDLE _fenceEvent;
	ID3D12Fence* _fence;
	UINT64 _fenceValue;
};
