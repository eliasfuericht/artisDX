#pragma once

#include "pch.h"
#include "Window.h"

class Renderer
{
	static const UINT backBufferCount = 2;

public:
	Renderer(Window window);

	~Renderer();

	CHECK Render();

private:
	CHECK InitializeAPI();
	CHECK CreateSwapchain(UINT w, UINT h);
	CHECK SetupSwapchain();
	IDXGISwapChain1* CreateSwapchain(	Window window, IDXGIFactory4* factory,
																		ID3D12CommandQueue* queue,
																		DXGI_SWAP_CHAIN_DESC1* swapchainDesc,
																		DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc = nullptr,
																		IDXGIOutput* output = nullptr);
	CHECK InitFrameBuffer();

	CHECK InitializeResources();
	CHECK CreateCommands();
	CHECK SetupCommands();

	// cleanup
	CHECK DestroyCommands();
	CHECK DestroyFrameBuffer();
	CHECK DestroyResources();
	CHECK DestroyAPI();

	Window _window;
	UINT _width;
	UINT _height;

	std::chrono::time_point<std::chrono::steady_clock> tStart, tEnd;
	float mElapsedTime = 0.0f;

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

	struct Vertex
	{
		float position[3];
		float color[3];
	};

	Vertex _vertexBufferData[3] = { {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
																 {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
																 {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}} };

	uint32_t _indexBufferData[3] = { 0, 1, 2 };

	struct 
	{
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMMATRIX viewMatrix;
		DirectX::XMMATRIX projectionMatrix;
	} _MVP;

	UINT _rtvDescriptorSize;
	ID3D12RootSignature* _rootSignature;
	ID3D12PipelineState* _pipelineState;

	// Sync
	UINT _frameIndex;
	HANDLE _fenceEvent;
	ID3D12Fence* _fence;
	UINT64 _fenceValue;
};
