#include "Application.h"

Application::Application(const char* name, int32_t w, int32_t h, uint32_t buffering, bool fullscreen)
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

	_name = name;
	_buffering = buffering;
	_fullscreen = fullscreen;
	
	try { 
		InitializeApplication(); 
	}
	catch (...)
	{
		ShutdownGraphics();
		Window::Shutdown();
		if (_comInitialized) CoUninitialize();
		throw;
	}
}

int Application::Start(int argc, char** argv, const char* name, uint32_t w, uint32_t h, uint32_t buffering, bool fullscreen)
{
	if (auto exitCode = Testing::HandleCommandLine(argc, argv))
		return *exitCode;

	try
	{
		Application application(name, w, h, buffering, fullscreen);
		application.Run();
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "Application failed: " << error.what() << '\n';
		return 1;
	}
}

void Application::InitializeApplication()
{
	Window::InitializeWindow(_name.c_str(), _width, _height, _fullscreen);

	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	_comInitialized = true;

#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::InitializeDebugController();
#endif

	D3D12Core::GraphicsDevice::InitializeFactory();
	D3D12Core::GraphicsDevice::InitializeAdapter();
	D3D12Core::GraphicsDevice::InitializeDevice();

#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::IntializeDebugDevice();
#endif

	_renderer.InitializeRenderer();

	D3D12Core::Swapchain::InitializeSwapchain();

	_renderer.InitializeResources();

	GUI::InitializeGUI();
	_frameCoordinator.Initialize(_buffering);
}

void Application::Run()
{
	_lastTime = std::chrono::high_resolution_clock::now();
	Window::Show();
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

		UpdateWindowFPS(deltaTime);
		_frameCoordinator.RenderFrame(_renderer, deltaTime);
	}

	_frameCoordinator.Shutdown();
	GUI::Shutdown();
}

void Application::ShutdownGraphics() noexcept
{
	try { _frameCoordinator.Shutdown(); }
	catch (const std::exception& error) { PRINT("Frame cleanup: ", error.what()); }
	try { GUI::Shutdown(); }
	catch (const std::exception& error) { PRINT("GUI cleanup: ", error.what()); }
}

void Application::UpdateWindowFPS(float dt)
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
		SetWindowTextA(Window::hWindow, title);
	}
}

Application::~Application()
{
	ShutdownGraphics();
	if (_comInitialized) CoUninitialize();
	Window::Shutdown();

	// Cleanup GUI
#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
	if (D3D12Core::GraphicsDevice::debugDevice) D3D12Core::GraphicsDevice::debugDevice.Reset();
	if (D3D12Core::GraphicsDevice::debugController) D3D12Core::GraphicsDevice::debugController.Reset();
#endif
	PRINT("SHUTDOWN");
}
