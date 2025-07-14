#include "GraphicsDevice.h"

namespace D3D12Core::GraphicsDevice
{

	MSWRL::ComPtr<IDXGIFactory4> _factory;
	MSWRL::ComPtr<IDXGIAdapter1> _adapter;
	MSWRL::ComPtr<ID3D12Device> _device;

#if defined(_DEBUG)
	MSWRL::ComPtr<ID3D12Debug1> _debugController;
	MSWRL::ComPtr<ID3D12DebugDevice> _debugDevice;
#endif

	void InitializeFactory(UINT flags)
	{
		ThrowIfFailed(CreateDXGIFactory2(flags, IID_PPV_ARGS(&_factory)), "Factory creation failed!");
	}

	void InitializeAdapter()
	{
		SIZE_T maxMemSize = 0;

		// iterate over all available adapters
		for (UINT adapterIndex = 0; ; ++adapterIndex)
		{
			MSWRL::ComPtr<IDXGIAdapter1> adapter;
			if (_factory->EnumAdapters1(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND)
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
					_adapter = adapter;
				}
			}
		}
	}

	void InitializeDevice()
	{
		ThrowIfFailed(D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device)), "Device creation failed!");
		_device->SetName(L"artisDX_Device");
	}

	MSWRL::ComPtr<IDXGIAdapter1> GetAdapter()
	{
		return _adapter;
	}

	MSWRL::ComPtr<IDXGIFactory4> GetFactory()
	{
		return _factory;
	}

	MSWRL::ComPtr<ID3D12Device> GetDevice()
	{
		return _device;
	}

#if defined(_DEBUG)
	void InitializeDebugController()
	{
		MSWRL::ComPtr<ID3D12Debug> debugBase;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugBase)), "DebugBase creation failed!");
		ThrowIfFailed(debugBase->QueryInterface(IID_PPV_ARGS(&_debugController)), "DebugController creation failed!");
		_debugController->EnableDebugLayer();
		_debugController->SetEnableGPUBasedValidation(TRUE);
	}

	void IntializeDebugDevice()
	{
		ThrowIfFailed(_device->QueryInterface(_debugDevice.GetAddressOf()), "DebugDevice creation failed!");
	}

	MSWRL::ComPtr<ID3D12Debug1> D3D12Core::GraphicsDevice::GetDebugController()
	{
		return _debugController;
	}

	MSWRL::ComPtr<ID3D12DebugDevice> D3D12Core::GraphicsDevice::GetDebugDevice()
	{
		return _debugDevice;
	}
#endif
}