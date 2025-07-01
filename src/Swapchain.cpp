#include "Swapchain.h"

UINT D3D12Core::Swapchain::_height;
UINT D3D12Core::Swapchain::_width;
MSWRL::ComPtr<IDXGISwapChain3> D3D12Core::Swapchain::_swapchain;
D3D12_VIEWPORT D3D12Core::Swapchain::_viewport;
D3D12_RECT D3D12Core::Swapchain::_surfaceSize;
UINT D3D12Core::Swapchain::_currentBuffer;
const UINT D3D12Core::Swapchain::_backBufferCount;
MSWRL::ComPtr<ID3D12DescriptorHeap> D3D12Core::Swapchain:: _rtvHeap;
UINT D3D12Core::Swapchain::_rtvDescriptorSize;
D3D12_CPU_DESCRIPTOR_HANDLE D3D12Core::Swapchain::_rtvDescriptor[_backBufferCount];
MSWRL::ComPtr<ID3D12Resource> D3D12Core::Swapchain::_renderTargets[_backBufferCount];
UINT D3D12Core::Swapchain::_frameIndex;

void D3D12Core::Swapchain::Init(Window& window)
{
	_width = window.GetWidth();
	_height = window.GetHeight();

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
	ThrowIfFailed(D3D12Core::GraphicsDevice::GetFactory()->CreateSwapChainForHwnd(D3D12Core::CommandQueue::GetCommandQueue().Get(), window.GetHWND(), &swapchainDesc, nullptr, nullptr, &swapchain), "Failed to create swapchain");

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
		_rtvDescriptor[i] = rtvHandle;
	}
}