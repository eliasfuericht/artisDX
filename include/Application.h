#pragma once

#include "pch.h"
#include "Window.h"
#include "Camera.h"
#include "ModelManager.h"

class Application
{
public:
	Application(const CHAR* name, INT w, INT h);
	void Run();
	~Application();

private:
	void InitDX12();
	void InitResources();
	void InitSwapchain(UINT w, UINT h);
	void InitCommands();
	void InitIMGUI();
	
	void Render();

	Window _window;
	Camera _camera;
	INT _width, _height;

	std::chrono::steady_clock::time_point _tLastTime = std::chrono::steady_clock::now();

	// DX12 Specific
	// Initialization
	MSWRL::ComPtr<IDXGIFactory4> _factory;
	MSWRL::ComPtr<IDXGIAdapter1> _adapter;
	MSWRL::ComPtr<ID3D12Device> _device;

#if defined(_DEBUG)
	MSWRL::ComPtr<ID3D12Debug1> _debugController;
	MSWRL::ComPtr<ID3D12DebugDevice> _debugDevice;
#endif

	MSWRL::ComPtr<ID3D12CommandQueue> _commandQueue;
	MSWRL::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;

	UINT _currentBuffer;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _rtvHeap;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _srvHeap;
	static const UINT _backBufferCount = 2; // double buffering
	D3D12_CPU_DESCRIPTOR_HANDLE  _rtvDescriptor[_backBufferCount] = {};
	MSWRL::ComPtr<ID3D12Resource> _renderTargets[_backBufferCount];
	MSWRL::ComPtr<IDXGISwapChain3> _swapchain;

	UINT _rtvDescriptorSize;
	MSWRL::ComPtr<ID3D12RootSignature> _rootSignature;
	MSWRL::ComPtr<ID3D12PipelineState> _pipelineState;

	// Sync
	UINT _frameIndex;
	HANDLE _fenceEvent;
	MSWRL::ComPtr<ID3D12Fence> _fence;
	UINT64 _fenceValue;

	// Resources
	D3D12_VIEWPORT _viewport;
	D3D12_RECT _surfaceSize;

	// DepthBuffer
	MSWRL::ComPtr<ID3D12Resource> _depthStencilBuffer;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _dsvHeap;

	MSWRL::ComPtr<ID3D12Resource> _uniformBuffer;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _uniformBufferHeap;
	// needs to be rawpointer since we perform memcpy on it
	UINT8* _mappedUniformBuffer;

	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;

	ModelManager _modelManager;

	struct
	{
		DirectX::XMFLOAT4X4 modelMatrix;
		DirectX::XMFLOAT4X4 viewMatrix;
		DirectX::XMFLOAT4X4 projectionMatrix;
	} _MVP;

	// IMGUI
	ImGuiIO* _imguiIO;
	BOOL _runImgui = false;
};