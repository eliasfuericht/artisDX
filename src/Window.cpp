#include "Window.h"

LRESULT CALLBACK WindowProcess(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam) {

	Window* windowInstance = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
		return true;

	switch (msg) {
		case WM_SIZE:
		{
			uint32_t newWidth = LOWORD(lParam);
			uint32_t newHeight = HIWORD(lParam);

			if (windowInstance && wParam != SIZE_MINIMIZED) 
			{
				D3D12Core::Swapchain::Resize(newWidth, newHeight);
				windowInstance->SetWidth(newWidth);
				windowInstance->SetHeight(newHeight);
			}
			return 0;
		}
		case WM_MOUSEMOVE:
		{
			if (!windowInstance->_captureMouse)
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
				windowInstance->HandleMouse(-deltaX, deltaY);

				ClientToScreen(hwnd, &center);
				SetCursorPos(center.x, center.y);
			}

			break;
		}
		case WM_LBUTTONDOWN:
		{
			if (!ImGui::IsAnyItemHovered() && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow))
			{
				if (!windowInstance->_captureMouse)
				{
					RECT clientRect;
					GetClientRect(hwnd, &clientRect);
					POINT center = { (clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2 };
					ClientToScreen(hwnd, &center);
					SetCursorPos(center.x, center.y);

					windowInstance->_captureMouse = true;
					SetCapture(windowInstance->GetHWND());
					ShowCursor(false);
				}
			}
			break;
		}
		case WM_KEYDOWN:
		{
			windowInstance->HandleKeys(wParam, WM_KEYDOWN);
			break;
		}
		case WM_KEYUP:
		{
			windowInstance->HandleKeys(wParam, WM_KEYUP);
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

void Window::HandleKeys(int32_t key, int32_t action)
{
	if (key >= 0 && key < 1024)
	{
		switch (action)
		{
			case WM_KEYDOWN:
			{
				_keys[key] = true;
				break;
			}
			case WM_KEYUP:
			{
				_keys[key] = false;
				break;
			}
			default:
				break;
		}
		if (key == KEYCODES::KEYCODE_ESC && _captureMouse)
		{
			_captureMouse = false;
			ShowCursor(true);
		}
	}
}

void Window::HandleMouse(float x, float y)
{
	_xChange += x;
	_yChange += y;
}

Window::Window(const char* title, uint32_t w, uint32_t h, bool fullscreen)
{
	if (fullscreen)
	{
		_windowWidth = GetSystemMetrics(SM_CXSCREEN);
		_windowHeight = GetSystemMetrics(SM_CYSCREEN);
		_windowMode = WS_POPUP;
	}
	else
	{
		_windowWidth = w;
		_windowHeight = h;
		_windowMode = WS_OVERLAPPEDWINDOW;
	}

	_windowTitle = title;

	_captureMouse = true;

	_xChange = 0.0f;
	_yChange = 0.0f;
	
	for (size_t i = 0; i < 1024; i++)
	{
		_keys[i] = 0;
	}
	
	Create();
}

void Window::Create()
{
	_windowClassEx.cbSize = sizeof(WNDCLASSEX);
	_windowClassEx.style = CS_HREDRAW | CS_VREDRAW;
	_windowClassEx.cbClsExtra = 0;
	_windowClassEx.cbWndExtra = 0;
	_windowClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
	_windowClassEx.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	_windowClassEx.hIcon = LoadIcon(0, IDI_APPLICATION);
	_windowClassEx.hIconSm = LoadIcon(0, IDI_APPLICATION);
	const char* windowClassName = "artisDXWindow";
	_windowClassEx.lpszClassName = windowClassName;
	_windowClassEx.lpszMenuName = nullptr;
	_windowClassEx.hInstance = GetHInstance();
	_windowClassEx.lpfnWndProc = WindowProcess;

	RegisterClassEx(&_windowClassEx);

	_hWindow = CreateWindow(windowClassName, _windowTitle, _windowMode, CW_USEDEFAULT, 0,
		_windowWidth, _windowHeight, nullptr, nullptr, GetHInstance(), nullptr);

	if (!_hWindow)
	{
		MessageBox(0, "Failed to create window", 0, 0);
		return;
	}

	SetWindowLongPtr(_hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

	ShowCursor(false);

	SetCapture(_hWindow);
}

void Window::Show()
{
	ShowWindow(_hWindow, SW_SHOW);
}

HWND Window::GetHWND() {
	return _hWindow;
}

float Window::GetXChange()
{
	float changeValueX = _xChange;
	_xChange = 0.0f;
	return changeValueX;
}

float Window::GetYChange()
{
	float changeValueY = _yChange;
	_yChange = 0.0f;
	return changeValueY;
}

void Window::Shutdown()
{
	if (_hWindow)
	{
		DestroyWindow(_hWindow);
		_hWindow = nullptr;
	}

	if (_windowClassEx.lpszClassName)
	{
		UnregisterClass(_windowClassEx.lpszClassName, _windowClassEx.hInstance);
	}
}