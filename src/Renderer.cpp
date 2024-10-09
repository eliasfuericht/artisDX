#include "Renderer.h"

Renderer::Renderer(std::weak_ptr<Window> window)
{
	_window = window;

	_factory = nullptr;
	_adapter = nullptr;

#if defined(_DEBUG)																	 
	_debugController = nullptr;
	_debugDevice = nullptr;
#endif					

	_device = nullptr;
	_commandQueue = nullptr;
	_commandAllocator = nullptr;
	_commandList = nullptr;

	_currentBuffer = 0;
	_rtvHeap = nullptr;
	for (size_t i = 0; i < backBufferCount; ++i)
	{
		_renderTargets[i] = nullptr;
	}
	_swapchain = nullptr;

	// Sync																																					 
	_frameIndex = 0;
	_fenceEvent = nullptr;
	_fence = nullptr;
	_fenceValue = 0;
	
	InitializeAPI();
}

CHECK Renderer::InitializeAPI()
{
	// Create Factory
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	ID3D12Debug* debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	ThrowIfFailed(debugController->QueryInterface(IID_PPV_ARGS(&_debugController)));
	_debugController->EnableDebugLayer();
	_debugController->SetEnableGPUBasedValidation(true);

	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

	debugController->Release();
	debugController = nullptr;
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&_factory)));

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != _factory->EnumAdapters1(adapterIndex, &_adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		_adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create
		// the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(_adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}

		// We won't use this adapter, so release it
		_adapter->Release();
	}

	// Create Device
	ID3D12Device* pDev = nullptr;
	ThrowIfFailed(D3D12CreateDevice(_adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device)));

	_device->SetName(L"artisDX");

#if defined(_DEBUG)
	// Get debug device
	ThrowIfFailed(_device->QueryInterface(&_debugDevice));
#endif

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)));

	// Create Command Allocator
	ThrowIfFailed(_device->CreateCommandAllocator( D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator)));

	// Sync
	ThrowIfFailed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));

	// Create Swapchain
	if (std::shared_ptr<Window> windowShared = _window.lock())
	{
		UINT width = windowShared->GetWidth();
		UINT height = windowShared->GetWidth();

		CreateSwapchain(width, height);
	}
	else
	{
		MessageBox(0, "Failed to access window", 0, 0);
		return NOTOK;
	}

	return OK;
}

CHECK Renderer::CreateSwapchain(UINT w, UINT h)
{
	_width = std::clamp(w, 1u, 0xffffu);
	_height = std::clamp(h, 1u, 0xffffu);

	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The
	// D3D12HelloFrameBuffering sample illustrates how to use fences for
	// efficient resource usage and to maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence = _fenceValue;
	ThrowIfFailed(_commandQueue->Signal(_fence, fence));
	_fenceValue++;

	// Wait until the previous frame is finished.
	if (_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(_fence->SetEventOnCompletion(fence, _fenceEvent));
		WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
	}

	DestroyFrameBuffer();
	SetupSwapchain();
	//initFrameBuffer();

	return OK;
}

CHECK Renderer::DestroyFrameBuffer()
{
	for (size_t i = 0; i < backBufferCount; ++i)
	{
		if (_renderTargets[i])
		{
			_renderTargets[i]->Release();
			_renderTargets[i] = 0;
		}
	}
	if (_rtvHeap)
	{
		_rtvHeap->Release();
		_rtvHeap = nullptr;
	}

	return OK;
}

CHECK Renderer::SetupSwapchain()
{
	_surfaceSize.left = 0;
	_surfaceSize.top = 0;
	_surfaceSize.right = static_cast<LONG>(_width);
	_surfaceSize.bottom = static_cast<LONG>(_height);

	_viewport.TopLeftX = 0.0f;
	_viewport.TopLeftY = 0.0f;
	_viewport.Width = static_cast<float>(_width);
	_viewport.Height = static_cast<float>(_height);
	_viewport.MinDepth = .1f;
	_viewport.MaxDepth = 1000.f;

	// Update Uniforms
	float zoom = 2.5f;

	// Update matrices
	uboVS.projectionMatrix =
		glm::perspective(45.0f, (float)_width / (float)_height, 0.01f, 1024.0f);

	uboVS.viewMatrix =
		glm::translate(glm::identity<mat4>(), vec3(0.0f, 0.0f, zoom));

	uboVS.modelMatrix = glm::identity<mat4>();

	if (_swapchain != nullptr)
	{
		_swapchain->ResizeBuffers(backBufferCount, _width, _height,
			DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	}
	else
	{
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.BufferCount = backBufferCount;
		swapchainDesc.Width = _width;
		swapchainDesc.Height = _height;
		swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchainDesc.SampleDesc.Count = 1;

		IDXGISwapChain1* swapchain = xgfx::createSwapchain(
			_window, _factory, _commandQueue, &swapchainDesc);
		HRESULT swapchainSupport = swapchain->QueryInterface(
			__uuidof(IDXGISwapChain3), (void**)&swapchain);
		if (SUCCEEDED(swapchainSupport))
		{
			_swapchain = (IDXGISwapChain3*)swapchain;
		}
	}
	_frameIndex = _swapchain->GetCurrentBackBufferIndex();
}