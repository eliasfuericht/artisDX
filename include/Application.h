#pragma once

#include "pch.h"
#include "Window.h"

class Application
{
public:
	Application(const CHAR* name, INT w, INT h);
	void Run();
	~Application();

private:
	void InitializeDX12();
	void InitializeResources();
	void Render();

	void SetupSwapchain(UINT w, UINT h);
	void SetupCommands();

	void DestroyFrameBuffer();
	void DestroyCommands();
	void DestroyResources();
	void DestroyAPI();

	Window _window;

	// DX12 Specific
	// Initialization
	IDXGIFactory4* _factory;
	IDXGIAdapter1* _adapter;
	ID3D12Device* _device;

#if defined(_DEBUG)
	ID3D12Debug1* _debugController;
	ID3D12DebugDevice* _debugDevice;
#endif

	ID3D12CommandQueue* _commandQueue;
	ID3D12CommandAllocator* _commandAllocator;
	ID3D12GraphicsCommandList* _commandList;

	UINT _currentBuffer;
	ID3D12DescriptorHeap* _rtvHeap;
	static const UINT backBufferCount = 2;
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