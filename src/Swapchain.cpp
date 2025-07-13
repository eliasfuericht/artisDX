#include "Swapchain.h"

UINT D3D12Core::Swapchain::_height;
UINT D3D12Core::Swapchain::_width;

MSWRL::ComPtr<IDXGISwapChain3> D3D12Core::Swapchain::_swapchain;
D3D12_VIEWPORT D3D12Core::Swapchain::_viewport;
D3D12_RECT D3D12Core::Swapchain::_surfaceSize;

UINT D3D12Core::Swapchain::_frameIndex;
UINT D3D12Core::Swapchain::_currentBuffer;
const UINT D3D12Core::Swapchain::_backBufferCount;

MSWRL::ComPtr<ID3D12DescriptorHeap> D3D12Core::Swapchain:: _rtvHeap;
UINT D3D12Core::Swapchain::_rtvDescriptorSize;
D3D12_CPU_DESCRIPTOR_HANDLE D3D12Core::Swapchain::_rtvCPUHandle[_backBufferCount];
MSWRL::ComPtr<ID3D12Resource> D3D12Core::Swapchain::_renderTargets[_backBufferCount];

MSWRL::ComPtr<ID3D12Resource> D3D12Core::Swapchain::_depthStencilBuffer;
MSWRL::ComPtr<ID3D12DescriptorHeap> D3D12Core::Swapchain::_dsvHeap;

BOOL D3D12Core::Swapchain::_windowResized = false;

void D3D12Core::Swapchain::InitializeSwapchain(INT width, INT height, HWND hwnd)
{
	_width = width;
	_height = height;

	_surfaceSize.left = 0;
	_surfaceSize.top = 0;
	_surfaceSize.right = static_cast<LONG>(_width);
	_surfaceSize.bottom = static_cast<LONG>(_height);

	_viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(_width), static_cast<float>(_height) };
	_viewport.MinDepth = 0.0f;
	_viewport.MaxDepth = 1.0f;

	// Create the swapchain if it doesn't exist
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
	swapchainDesc.BufferCount = _backBufferCount;
	swapchainDesc.Width = _width;
	swapchainDesc.Height = _height;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchainDesc.SampleDesc.Count = 1;

	MSWRL::ComPtr<IDXGISwapChain1> swapchain;
	ThrowIfFailed(D3D12Core::GraphicsDevice::GetFactory()->CreateSwapChainForHwnd(D3D12Core::CommandQueue::_commandQueue.Get(), hwnd, &swapchainDesc, nullptr, nullptr, &swapchain), "Failed to create swapchain");

	MSWRL::ComPtr<IDXGISwapChain3> swapchain3;
	ThrowIfFailed(swapchain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapchain3), "QueryInterface for swapchain failed.");
	_swapchain = swapchain3;

	_frameIndex = _swapchain->GetCurrentBackBufferIndex();

	// Recreate descriptor heaps and render targets
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = _backBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));

	_rtvDescriptorSize = D3D12Core::GraphicsDevice::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create frame resources
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < _backBufferCount; i++)
	{
		ThrowIfFailed(_swapchain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i])));
		D3D12Core::GraphicsDevice::GetDevice()->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += _rtvDescriptorSize;
		_rtvCPUHandle[i] = rtvHandle;
	}

	CreateDepthBuffer(width, height);
}

void D3D12Core::Swapchain::CreateDepthBuffer(INT newWidth, INT newHeight)
{
	// Heap properties for creating the texture (GPU read/write)
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	// Create Depth-Stencil Resource (Texture2D)
	D3D12_RESOURCE_DESC depthResourceDesc = {};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Alignment = 0;
	depthResourceDesc.Width = newWidth;
	depthResourceDesc.Height = newHeight;
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.MipLevels = 1;
	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.SampleDesc.Quality = 0;
	depthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&_depthStencilBuffer)
	));

	// Create the DSV Heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap)));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	// Create the DSV for the depth-stencil buffer
	D3D12Core::GraphicsDevice::GetDevice()->CreateDepthStencilView(_depthStencilBuffer.Get(), &dsvDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void D3D12Core::Swapchain::Resize(INT newWidth, INT newHeight) 
{
	_windowResized = true;
	// Flush GPU to avoid touching resources in use
	D3D12Core::CommandQueue::WaitForFence();

	// Release old views
	for (UINT i = 0; i < _backBufferCount; ++i)
	{
		_renderTargets[i].Reset();
	}

	DXGI_SWAP_CHAIN_DESC desc = {};
	_swapchain->GetDesc(&desc);

	HRESULT hr = _swapchain->ResizeBuffers(_backBufferCount, newWidth, newHeight,
		desc.BufferDesc.Format, desc.Flags);
	assert(SUCCEEDED(hr));

	_frameIndex = _swapchain->GetCurrentBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	// Recreate render target views
	for (UINT i = 0; i < _backBufferCount; ++i) 
	{
		ThrowIfFailed(_swapchain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i])));
		D3D12Core::GraphicsDevice::GetDevice()->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += _rtvDescriptorSize;
		_rtvCPUHandle[i] = rtvHandle;
	}

	// Recreate depth buffer (if needed)
	CreateDepthBuffer(newWidth, newHeight);

	// Update viewport and scissor rect
	_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(newWidth), static_cast<float>(newHeight));
	_surfaceSize = { 0, 0, static_cast<LONG>(newWidth), static_cast<LONG>(newHeight) };
}
