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

	Window _window;

	// DX12 Specific
	// Initialization
	MSPTR::ComPtr<IDXGIFactory4> _factory;
	MSPTR::ComPtr<IDXGIAdapter1> _adapter;
	MSPTR::ComPtr<ID3D12Device> _device;

#if defined(_DEBUG)
	MSPTR::ComPtr<ID3D12Debug1> _debugController;
	MSPTR::ComPtr<ID3D12DebugDevice> _debugDevice;
#endif

	MSPTR::ComPtr<ID3D12CommandQueue> _commandQueue;
	MSPTR::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	MSPTR::ComPtr<ID3D12GraphicsCommandList> _commandList;

	UINT _currentBuffer;
	MSPTR::ComPtr<ID3D12DescriptorHeap> _rtvHeap;
	static const UINT backBufferCount = 2;
	MSPTR::ComPtr<ID3D12Resource> _renderTargets[backBufferCount];
	MSPTR::ComPtr<IDXGISwapChain3> _swapchain;


	UINT _rtvDescriptorSize;
	MSPTR::ComPtr<ID3D12RootSignature> _rootSignature;
	MSPTR::ComPtr<ID3D12PipelineState> _pipelineState;

	// Sync
	UINT _frameIndex;
	HANDLE _fenceEvent;
	MSPTR::ComPtr<ID3D12Fence> _fence;
	UINT64 _fenceValue;

	// Resources
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _surfaceSize;

	MSPTR::ComPtr<ID3D12Resource> _vertexBuffer;
	MSPTR::ComPtr<ID3D12Resource> _indexBuffer;

	MSPTR::ComPtr<ID3D12Resource> _uniformBuffer;
	MSPTR::ComPtr<ID3D12DescriptorHeap> _uniformBufferHeap;
	// needs to be rawpointer since we perform memcpy on it
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
};