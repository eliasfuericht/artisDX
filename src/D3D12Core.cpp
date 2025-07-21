#include "D3D12Core.h"

namespace D3D12Core
{
	namespace GraphicsDevice
	{
		MSWRL::ComPtr<IDXGIFactory4> factory;
		MSWRL::ComPtr<IDXGIAdapter1> adapter;
		MSWRL::ComPtr<ID3D12Device> device;

#if defined(_DEBUG)
		MSWRL::ComPtr<ID3D12Debug1> debugController;
		MSWRL::ComPtr<ID3D12DebugDevice> debugDevice;
#endif

		void InitializeFactory()
		{
			uint32_t flags = 0;
#if defined(_DEBUG)
			flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
			ThrowIfFailed(CreateDXGIFactory2(flags, IID_PPV_ARGS(&D3D12Core::GraphicsDevice::factory)), "Factory creation failed!");
		}

		void InitializeAdapter()
		{
			if (!D3D12Core::GraphicsDevice::factory)
				InitializeFactory();

			SIZE_T maxMemSize = 0;

			// iterate over all available adapters
			for (uint32_t adapterIndex = 0; ; ++adapterIndex)
			{
				MSWRL::ComPtr<IDXGIAdapter1> adapter;
				if (D3D12Core::GraphicsDevice::factory->EnumAdapters1(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND)
					break;

				DXGI_ADAPTER_DESC1 desc;
				adapter->GetDesc1(&desc);

				// Skip software adapters
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
					continue;

				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
				{
					if (desc.DedicatedVideoMemory > maxMemSize)
					{
						maxMemSize = desc.DedicatedVideoMemory;
						D3D12Core::GraphicsDevice::adapter = adapter;
					}
				}
			}
		}

		void InitializeDevice()
		{
			if (!D3D12Core::GraphicsDevice::adapter)
				InitializeAdapter();

			ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&D3D12Core::GraphicsDevice::device)), "Device creation failed!");
			D3D12Core::GraphicsDevice::device->SetName(L"artisDX_Device");
		}

#if defined(_DEBUG)
		void InitializeDebugController()
		{
			MSWRL::ComPtr<ID3D12Debug> debugBase;
			ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugBase)), "DebugBase creation failed!");
			ThrowIfFailed(debugBase->QueryInterface(IID_PPV_ARGS(&D3D12Core::GraphicsDevice::debugController)), "DebugController creation failed!");
			D3D12Core::GraphicsDevice::debugController->EnableDebugLayer();
			D3D12Core::GraphicsDevice::debugController->SetEnableGPUBasedValidation(true);
		}

		void IntializeDebugDevice()
		{
			ThrowIfFailed(D3D12Core::GraphicsDevice::device->QueryInterface(D3D12Core::GraphicsDevice::debugDevice.GetAddressOf()), "DebugDevice creation failed!");
		}
#endif
	}

	namespace Swapchain
	{
		uint32_t width;
		uint32_t height;

		MSWRL::ComPtr<IDXGISwapChain3> swapchain;
		D3D12_VIEWPORT viewport;
		D3D12_RECT surfaceSize;

		uint32_t frameIndex;
		uint32_t currentBuffer;

		MSWRL::ComPtr<ID3D12DescriptorHeap> rtvHeap;
		uint32_t rtvDescriptorSize;
		D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle[backBufferCount];
		MSWRL::ComPtr<ID3D12Resource> renderTargets[backBufferCount];

		MSWRL::ComPtr<ID3D12Resource> depthStencilBuffer;
		MSWRL::ComPtr<ID3D12DescriptorHeap> dsvHeap;

		bool windowResized;

		void InitializeSwapchain(int32_t width, int32_t height, HWND hwnd)
		{
			D3D12Core::Swapchain::width = width;
			D3D12Core::Swapchain::height = height;

			D3D12Core::Swapchain::surfaceSize.left = 0;
			D3D12Core::Swapchain::surfaceSize.top = 0;
			D3D12Core::Swapchain::surfaceSize.right = static_cast<long>(D3D12Core::Swapchain::width);
			D3D12Core::Swapchain::surfaceSize.bottom = static_cast<long>(D3D12Core::Swapchain::height);

			D3D12Core::Swapchain::viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(D3D12Core::Swapchain::width), static_cast<float>(D3D12Core::Swapchain::height) };
			D3D12Core::Swapchain::viewport.MinDepth = 0.0f;
			D3D12Core::Swapchain::viewport.MaxDepth = 1.0f;

			// Create the swapchain if it doesn't exist
			DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
			swapchainDesc.BufferCount = D3D12Core::Swapchain::backBufferCount;
			swapchainDesc.Width = D3D12Core::Swapchain::width;
			swapchainDesc.Height = D3D12Core::Swapchain::height;
			swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapchainDesc.SampleDesc.Count = 1;

			MSWRL::ComPtr<IDXGISwapChain1> swapchain;
			ThrowIfFailed(D3D12Core::GraphicsDevice::factory->CreateSwapChainForHwnd(CommandQueueManager::GetCommandQueue(QUEUETYPE::QUEUE_GRAPHICS)._commandQueue.Get(), hwnd, &swapchainDesc, nullptr, nullptr, &swapchain), "Failed to create swapchain");
			
			MSWRL::ComPtr<IDXGISwapChain3> swapchain3;
			ThrowIfFailed(swapchain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapchain3), "QueryInterface for swapchain failed.");
			D3D12Core::Swapchain::swapchain = swapchain3;

			D3D12Core::Swapchain::frameIndex = D3D12Core::Swapchain::swapchain->GetCurrentBackBufferIndex();

			// Recreate descriptor heaps and render targets
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.NumDescriptors = D3D12Core::Swapchain::backBufferCount;
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&D3D12Core::Swapchain::rtvHeap)));

			D3D12Core::Swapchain::rtvDescriptorSize = D3D12Core::GraphicsDevice::device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			// Create frame resources
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(D3D12Core::Swapchain::rtvHeap->GetCPUDescriptorHandleForHeapStart());
			for (size_t i = 0; i < D3D12Core::Swapchain::backBufferCount; i++)
			{
				ThrowIfFailed(D3D12Core::Swapchain::swapchain->GetBuffer(i, IID_PPV_ARGS(&D3D12Core::Swapchain::renderTargets[i])));
				D3D12Core::GraphicsDevice::device->CreateRenderTargetView(D3D12Core::Swapchain::renderTargets[i].Get(), nullptr, rtvHandle);
				rtvHandle.ptr += D3D12Core::Swapchain::rtvDescriptorSize;
				D3D12Core::Swapchain::rtvCPUHandle[i] = rtvHandle;
			}

			CreateDepthBuffer(width, height);
		}

		void CreateDepthBuffer(int32_t newWidth, int32_t newHeight)
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

			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&depthResourceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&D3D12Core::Swapchain::depthStencilBuffer)
			));

			// Create the DSV Heap
			D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
			dsvHeapDesc.NumDescriptors = 1;
			dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&D3D12Core::Swapchain::dsvHeap)));

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

			// Create the DSV for the depth-stencil buffer
			D3D12Core::GraphicsDevice::device->CreateDepthStencilView(D3D12Core::Swapchain::depthStencilBuffer.Get(), &dsvDesc, D3D12Core::Swapchain::dsvHeap->GetCPUDescriptorHandleForHeapStart());
		}

		void D3D12Core::Swapchain::Resize(int32_t newWidth, int32_t newHeight)
		{
			D3D12Core::Swapchain::windowResized = true;
			// Flush GPU to avoid touching resources in use
			CommandQueueManager::GetCommandQueue(QUEUETYPE::QUEUE_GRAPHICS).WaitForFence();

			// Release old views
			for (size_t i = 0; i < D3D12Core::Swapchain::backBufferCount; ++i)
			{
				D3D12Core::Swapchain::renderTargets[i].Reset();
			}

			DXGI_SWAP_CHAIN_DESC desc = {};
			D3D12Core::Swapchain::swapchain->GetDesc(&desc);

			HRESULT hr = D3D12Core::Swapchain::swapchain->ResizeBuffers(D3D12Core::Swapchain::backBufferCount, newWidth, newHeight,
				desc.BufferDesc.Format, desc.Flags);
			assert(SUCCEEDED(hr));

			D3D12Core::Swapchain::frameIndex = D3D12Core::Swapchain::swapchain->GetCurrentBackBufferIndex();

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(D3D12Core::Swapchain::rtvHeap->GetCPUDescriptorHandleForHeapStart());
			// Recreate render target views
			for (size_t i = 0; i < D3D12Core::Swapchain::backBufferCount; ++i)
			{
				ThrowIfFailed(D3D12Core::Swapchain::swapchain->GetBuffer(i, IID_PPV_ARGS(&D3D12Core::Swapchain::renderTargets[i])));
				D3D12Core::GraphicsDevice::device->CreateRenderTargetView(D3D12Core::Swapchain::renderTargets[i].Get(), nullptr, rtvHandle);
				rtvHandle.ptr += D3D12Core::Swapchain::rtvDescriptorSize;
				D3D12Core::Swapchain::rtvCPUHandle[i] = rtvHandle;
			}

			// Recreate depth buffer (if needed)
			CreateDepthBuffer(newWidth, newHeight);

			// Update viewport and scissor rect
			D3D12Core::Swapchain::viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(newWidth), static_cast<float>(newHeight));
			D3D12Core::Swapchain::surfaceSize = { 0, 0, static_cast<long>(newWidth), static_cast<long>(newHeight) };
		}
	}

	namespace ShaderCompiler
	{
		Microsoft::WRL::ComPtr<IDxcUtils> utils;
		Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;

		void InitializeShaderCompiler()
		{
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&D3D12Core::ShaderCompiler::utils)));
			ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&D3D12Core::ShaderCompiler::compiler)));
			ThrowIfFailed(D3D12Core::ShaderCompiler::utils->CreateDefaultIncludeHandler(&D3D12Core::ShaderCompiler::includeHandler));
		}
	}
}