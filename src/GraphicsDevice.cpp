#include "GraphicsDevice.h"

MSWRL::ComPtr<IDXGIFactory4> D3D12Core::GraphicsDevice::_factory = nullptr;
MSWRL::ComPtr<IDXGIAdapter1> D3D12Core::GraphicsDevice::_adapter = nullptr;
MSWRL::ComPtr<ID3D12Device> D3D12Core::GraphicsDevice::_device = nullptr;

void D3D12Core::GraphicsDevice::InitializeFactory(UINT flags)
{
	ThrowIfFailed(CreateDXGIFactory2(flags, IID_PPV_ARGS(&_factory)), "Factory creation failed!");
}

void D3D12Core::GraphicsDevice::InitializeAdapter()
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

void D3D12Core::GraphicsDevice::InitializeDevice()
{
	ThrowIfFailed(D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device)), "Device creation failed!");
	_device->SetName(L"artisDX_Device");
}

MSWRL::ComPtr<IDXGIAdapter1> D3D12Core::GraphicsDevice::GetAdapter()
{
	return _adapter;
}

MSWRL::ComPtr<IDXGIFactory4> D3D12Core::GraphicsDevice::GetFactory()
{
	return _factory;
}

MSWRL::ComPtr<ID3D12Device> D3D12Core::GraphicsDevice::GetDevice()
{
	return _device;
}

#if defined(_DEBUG)
MSWRL::ComPtr<ID3D12Debug1> D3D12Core::GraphicsDevice::_debugController = nullptr;
MSWRL::ComPtr<ID3D12DebugDevice> D3D12Core::GraphicsDevice::_debugDevice = nullptr;

void D3D12Core::GraphicsDevice::InitializeDebugController()
{
	MSWRL::ComPtr<ID3D12Debug> debugBase;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugBase)), "DebugBase creation failed!");
	ThrowIfFailed(debugBase->QueryInterface(IID_PPV_ARGS(&_debugController)), "DebugController creation failed!");
	_debugController->EnableDebugLayer();
	_debugController->SetEnableGPUBasedValidation(TRUE);
}

void D3D12Core::GraphicsDevice::IntializeDebugDevice()
{
	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->QueryInterface(_debugDevice.GetAddressOf()), "DebugDevice creation failed!");
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