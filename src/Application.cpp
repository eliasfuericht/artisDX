#include "Application.h"

Application::Application(const char* name, int32_t w, int32_t h, bool fullscreen)
	: _window(name, w, h, fullscreen)
{
	if (fullscreen)
	{
		_width = GetSystemMetrics(SM_CXSCREEN);
		_height = GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{
		_width = w;
		_height = h;
	}																													 

	InitializeApplication();
	GUI::InitializeGUI(_window.GetHWND());
}

void Application::InitializeApplication()
{
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::InitializeDebugController();
#endif

	D3D12Core::GraphicsDevice::InitializeFactory();
	D3D12Core::GraphicsDevice::InitializeAdapter();
	D3D12Core::GraphicsDevice::InitializeDevice();

#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::IntializeDebugDevice();
#endif

	_renderer.InitializeRenderer(&_window);
}

void Application::Run()
{
	_lastTime = std::chrono::high_resolution_clock::now();
	_window.Show();
	MSG msg = { 0 };

	bool running = true;

	while (running)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!running)
			break;

		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		std::chrono::duration<float> dt = now - _tLastTime;
		_tLastTime = now;
		float deltaTime = dt.count();

		Update(deltaTime);
		_renderer.Render(deltaTime);
		GUI::Draw();
		Present();
	}

	_renderer.Shutdown();
	GUI::Shutdown();
}

void Application::Update(float dt)
{
	_elapsedTime += dt;
	_frameCount++;

	if (_elapsedTime >= 1.0)
	{
		_fps = _frameCount / _elapsedTime; 
		_frameCount = 0;
		_elapsedTime = 0.0;
		char title[256];
		sprintf_s(title, "FPS: %.2f", _fps);
		SetWindowTextA(_window.GetHWND(), title);
	}
}

void Application::Present()
{
	D3D12Core::Swapchain::swapchain->Present(0, 0);

	D3D12Core::Swapchain::frameIndex = D3D12Core::Swapchain::swapchain->GetCurrentBackBufferIndex();
}

Application::~Application()
{
	CoUninitialize();
	_window.Shutdown();

	// Cleanup GUI
#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
	if (D3D12Core::GraphicsDevice::debugDevice) D3D12Core::GraphicsDevice::debugDevice.Reset();
	if (D3D12Core::GraphicsDevice::debugController) D3D12Core::GraphicsDevice::debugController.Reset();
#endif
	PRINT("SHUTDOWN");
}