#pragma once

#include "pch.h"
#include "Window.h"
#include "Camera.h"

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
	Camera _camera;

	// DX12 Specific
	// Initialization
	MS::ComPtr<IDXGIFactory4> _factory;
	MS::ComPtr<IDXGIAdapter1> _adapter;
	MS::ComPtr<ID3D12Device> _device;

#if defined(_DEBUG)
	MS::ComPtr<ID3D12Debug1> _debugController;
	MS::ComPtr<ID3D12DebugDevice> _debugDevice;
#endif

	MS::ComPtr<ID3D12CommandQueue> _commandQueue;
	MS::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	MS::ComPtr<ID3D12GraphicsCommandList> _commandList;

	UINT _currentBuffer;
	MS::ComPtr<ID3D12DescriptorHeap> _rtvHeap;
	static const UINT backBufferCount = 2;
	MS::ComPtr<ID3D12Resource> _renderTargets[backBufferCount];
	MS::ComPtr<IDXGISwapChain3> _swapchain;


	UINT _rtvDescriptorSize;
	MS::ComPtr<ID3D12RootSignature> _rootSignature;
	MS::ComPtr<ID3D12PipelineState> _pipelineState;

	// Sync
	UINT _frameIndex;
	HANDLE _fenceEvent;
	MS::ComPtr<ID3D12Fence> _fence;
	UINT64 _fenceValue;

	// Resources
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _surfaceSize;

	MS::ComPtr<ID3D12Resource> _vertexBuffer;
	MS::ComPtr<ID3D12Resource> _indexBuffer;

	MS::ComPtr<ID3D12Resource> _uniformBuffer;
	MS::ComPtr<ID3D12DescriptorHeap> _uniformBufferHeap;
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