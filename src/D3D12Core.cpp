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
				MSWRL::ComPtr<IDXGIAdapter1> adapter1;
				if (D3D12Core::GraphicsDevice::factory->EnumAdapters1(adapterIndex, &adapter1) == DXGI_ERROR_NOT_FOUND)
					break;

				DXGI_ADAPTER_DESC1 desc;
				adapter1->GetDesc1(&desc);

				// Skip software adapters
				if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
					continue;

				if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
				{
					if (desc.DedicatedVideoMemory > maxMemSize)
					{
						maxMemSize = desc.DedicatedVideoMemory;
						D3D12Core::GraphicsDevice::adapter = adapter1;
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

		D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUHandle[backBufferCount];
		MSWRL::ComPtr<ID3D12Resource> renderTargets[backBufferCount];

		void InitializeSwapchain()
		{
			D3D12Core::Swapchain::width = Window::width;
			D3D12Core::Swapchain::height = Window::height;

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

			MSWRL::ComPtr<IDXGISwapChain1> swapchain1;
			ThrowIfFailed(D3D12Core::GraphicsDevice::factory->CreateSwapChainForHwnd(CommandQueueManager::GetCommandQueue(QUEUETYPE::QUEUE_GRAPHICS)._commandQueue.Get(), Window::hWindow, &swapchainDesc, nullptr, nullptr, &swapchain1), "Failed to create swapchain");
			
			MSWRL::ComPtr<IDXGISwapChain3> swapchain3;
			ThrowIfFailed(swapchain1->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapchain3), "QueryInterface for swapchain failed.");
			D3D12Core::Swapchain::swapchain = swapchain3;

			for (size_t i = 0; i < D3D12Core::Swapchain::backBufferCount; i++)
			{
				D3D12Core::Swapchain::rtvCPUHandle[i] = DescriptorAllocator::RTV::Allocate();
				ThrowIfFailed(D3D12Core::Swapchain::swapchain->GetBuffer(static_cast<uint32_t>(i), IID_PPV_ARGS(&D3D12Core::Swapchain::renderTargets[i])));
				D3D12Core::GraphicsDevice::device->CreateRenderTargetView(D3D12Core::Swapchain::renderTargets[i].Get(), nullptr, rtvCPUHandle[i]);
			}
		}

		void D3D12Core::Swapchain::Resize(int32_t newWidth, int32_t newHeight)
		{
			CommandQueueManager::GetCommandQueue(QUEUETYPE::QUEUE_GRAPHICS).WaitForFence();

			for (size_t i = 0; i < D3D12Core::Swapchain::backBufferCount; ++i)
			{
				D3D12Core::Swapchain::renderTargets[i].Reset();
			}

			DXGI_SWAP_CHAIN_DESC desc = {};
			D3D12Core::Swapchain::swapchain->GetDesc(&desc);

			ThrowIfFailed(D3D12Core::Swapchain::swapchain->ResizeBuffers(D3D12Core::Swapchain::backBufferCount, newWidth, newHeight,desc.BufferDesc.Format, desc.Flags), "Resizing Swapchain failed!");

			for (size_t i = 0; i < D3D12Core::Swapchain::backBufferCount; ++i)
			{
				ThrowIfFailed(D3D12Core::Swapchain::swapchain->GetBuffer(static_cast<uint32_t>(i), IID_PPV_ARGS(&D3D12Core::Swapchain::renderTargets[i])));
				D3D12Core::GraphicsDevice::device->CreateRenderTargetView(D3D12Core::Swapchain::renderTargets[i].Get(), nullptr, D3D12Core::Swapchain::rtvCPUHandle[i]);
			}

			D3D12Core::Swapchain::viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(newWidth), static_cast<float>(newHeight));
			D3D12Core::Swapchain::surfaceSize = { 0, 0, static_cast<long>(newWidth), static_cast<long>(newHeight) };
		
			CommandQueueManager::GetCommandQueue(QUEUETYPE::QUEUE_GRAPHICS).WaitForFence();
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