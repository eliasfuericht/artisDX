#include "Window.h"

LRESULT CALLBACK WindowProcess(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) 
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	switch (msg) {
		case WM_SIZE:
		{
			uint32_t newWidth = LOWORD(lParam);
			uint32_t newHeight = HIWORD(lParam);

			if (Window::initialized && wParam != SIZE_MINIMIZED)
			{
				D3D12Core::Swapchain::Resize(newWidth, newHeight);
				Window::width = newWidth;
				Window::height = newHeight;
			}
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			if (!Window::initialized || !Window::captureMouse)
				break;

			POINT currentCursorPos;
			GetCursorPos(&currentCursorPos);
			ScreenToClient(hwnd, &currentCursorPos);
			RECT clientRect;
			GetClientRect(hwnd, &clientRect);
			POINT center = { (clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2 };

			auto deltaX = center.x - currentCursorPos.x;
			auto deltaY = center.y - currentCursorPos.y;

			if (abs(deltaX) > 0 || abs(deltaY) > 0)
			{
				Window::HandleMouse(-deltaX, deltaY);

				ClientToScreen(hwnd, &center);
				SetCursorPos(center.x, center.y);
			}

			break;
		}
		case WM_KEYDOWN:
		{
			Window::HandleKeys(wParam, WM_KEYDOWN);
			break;
		}
		case WM_KEYUP:
		{
			Window::HandleKeys(wParam, WM_KEYUP);
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

namespace Window
{
	WNDCLASSEX windowClassEx;
	const char* windowTitle;
	ULONG windowMode;
	uint32_t width;
	uint32_t height;
	HWND hWindow;

	bool initialized = false;
	bool captureMouse = false;
	bool keys[1024];
	float xChange;
	float yChange;

	void InitializeWindow(const char* title, uint32_t w, uint32_t h, bool fullscreen)
	{
		if (fullscreen)
		{
			Window::width = GetSystemMetrics(SM_CXSCREEN);
			Window::height = GetSystemMetrics(SM_CYSCREEN);
			Window::windowMode = WS_POPUP;
		}
		else
		{
			Window::width = w;
			Window::height = h;
			Window::windowMode = WS_OVERLAPPEDWINDOW;
		}

		Window::windowTitle = title;

		Window::captureMouse = true;

		Window::xChange = 0.0f;
		Window::yChange = 0.0f;

		for (size_t i = 0; i < 1024; i++)
		{
			Window::keys[i] = 0;
		}

		Create();
	}

	void HandleKeys(int32_t key, int32_t action)
	{
		if (key >= 0 && key < 1024)
		{
			switch (action)
			{
			case WM_KEYDOWN:
			{
				Window::keys[key] = true;
				break;
			}
			case WM_KEYUP:
			{
				Window::keys[key] = false;
				break;
			}
			default:
				break;
			}
			if (key == KEYCODES::KEYCODE_ESC && Window::captureMouse)
			{
				Window::captureMouse = false;
				ShowCursor(true);
			}
		}
	}

	void Window::HandleMouse(float x, float y)
	{
		Window::xChange += x;
		Window::yChange += y;
	}
	
	void Window::Create()
	{
		Window::windowClassEx.cbSize = sizeof(WNDCLASSEX);
		Window::windowClassEx.style = CS_HREDRAW | CS_VREDRAW;
		Window::windowClassEx.cbClsExtra = 0;
		Window::windowClassEx.cbWndExtra = 0;
		Window::windowClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
		Window::windowClassEx.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		Window::windowClassEx.hIcon = LoadIcon(0, IDI_APPLICATION);
		Window::windowClassEx.hIconSm = LoadIcon(0, IDI_APPLICATION);
		const char* windowClassName = "artisDXWindow";
		Window::windowClassEx.lpszClassName = windowClassName;
		Window::windowClassEx.lpszMenuName = nullptr;
		Window::windowClassEx.hInstance = GetHInstance();
		Window::windowClassEx.lpfnWndProc = WindowProcess;

		RegisterClassEx(&Window::windowClassEx);

		Window::hWindow = CreateWindow(windowClassName, Window::windowTitle, Window::windowMode, CW_USEDEFAULT, 0,
			Window::width, Window::height, nullptr, nullptr, GetHInstance(), nullptr);

		if (!Window::hWindow)
		{
			MessageBox(0, "Failed to create window", 0, 0);
			return;
		}

		Window::initialized = true;

		//SetWindowLongPtr(Window::hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

		ShowCursor(false);

		SetCapture(Window::hWindow);
	}

	void Show()
	{
		ShowWindow(Window::hWindow, SW_SHOW);
	}

	float GetXChange()
	{
		float changeValueX = Window::xChange;
		Window::xChange = 0.0f;
		return changeValueX;
	}

	float GetYChange()
	{
		float changeValueY = Window::yChange;
		Window::yChange = 0.0f;
		return changeValueY;
	}

	void Shutdown()
	{
		if (Window::hWindow)
		{
			DestroyWindow(Window::hWindow);
			Window::hWindow = nullptr;
		}

		if (Window::windowClassEx.lpszClassName)
		{
			UnregisterClass(Window::windowClassEx.lpszClassName, Window::windowClassEx.hInstance);
		}
	}
}
