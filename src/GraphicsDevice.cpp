#include "GraphicsDevice.h"

MSWRL::ComPtr<IDXGIFactory4> D3D12Core::GraphicsDevice::_factory = nullptr;
MSWRL::ComPtr<IDXGIAdapter1> D3D12Core::GraphicsDevice::_adapter = nullptr;
MSWRL::ComPtr<ID3D12Device> D3D12Core::GraphicsDevice::_device = nullptr;

void D3D12Core::GraphicsDevice::InitializeFactory(MSWRL::ComPtr<IDXGIFactory4> factory)
{
	_factory = factory;
}

void D3D12Core::GraphicsDevice::InitializeAdapter(MSWRL::ComPtr<IDXGIAdapter1> adapter)
{
	_adapter = adapter;
}

void D3D12Core::GraphicsDevice::InitializeDevice(MSWRL::ComPtr<ID3D12Device> device)
{
	_device = device;
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

void D3D12Core::GraphicsDevice::InitializeDebugController(MSWRL::ComPtr<ID3D12Debug1> debugController)
{
	_debugController = debugController;
}

void D3D12Core::GraphicsDevice::IntializeDebugDevice(MSWRL::ComPtr<ID3D12DebugDevice> debugDevice)
{
	_debugDevice = debugDevice;
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