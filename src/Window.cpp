#include "Window.h"

LRESULT CALLBACK WindowProcess(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	Window* windowInstance = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (msg) {
		case WM_MOUSEMOVE:
		{
			if (!windowInstance->_captureMouse)
				break;

			// Get the current cursor position (in screen coordinates)
			POINT currentCursorPos;
			GetCursorPos(&currentCursorPos);

			// Convert screen coordinates to client coordinates
			ScreenToClient(hwnd, &currentCursorPos);

			// Calculate mouse deltas (movement since last frame)
			RECT clientRect;
			GetClientRect(hwnd, &clientRect);
			POINT center = { (clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2 };

			// The delta is how much the cursor moved from the center
			LONG deltaX = center.x - currentCursorPos.x;
			LONG deltaY = center.y - currentCursorPos.y;

			windowInstance->HandleMouse(deltaX, deltaY);

			ClientToScreen(hwnd, &center);
			SetCursorPos(center.x, center.y);

			break;
		}
		case WM_LBUTTONDOWN:
		{
			if (!windowInstance->_captureMouse)
			{
				windowInstance->_captureMouse = true;
				SetCapture(windowInstance->GetHWND());
				ShowCursor(FALSE);
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
		case WM_SYSCOMMAND:
		{
			if ((wParam & 0xFFF0) == SC_CLOSE)  // SC_CLOSE is triggered by Alt + F4
			{
				// Perform any additional logic or just allow default behavior
				PostMessage(hwnd, WM_CLOSE, 0, 0);  // Post WM_CLOSE to close the window
				return 0;
			}
			break;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

void Window::HandleKeys(INT key, INT action)
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
		}

		if (key == KEYCODES::ESC && _captureMouse)
		{
			_captureMouse = false;
			SetCapture(0);
			ShowCursor(TRUE);
		}
	}
}

void Window::HandleMouse(FLOAT x, FLOAT y)
{
	_xChange += x;
	_yChange += y;
}

Window::Window(const CHAR* title, UINT w, UINT h)
	: _windowTitle(title), _windowWidth(w), _windowHeight(h)
{
	for (INT i = 0; i < 1024; i++)
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
	const CHAR* windowClassName = "artisDXWindow";
	_windowClassEx.lpszClassName = windowClassName;
	_windowClassEx.lpszMenuName = nullptr;
	_windowClassEx.hInstance = GetHInstance();
	_windowClassEx.lpfnWndProc = WindowProcess;

	RegisterClassEx(&_windowClassEx);

	_hWindow = CreateWindow(windowClassName, _windowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
															_windowWidth, _windowHeight, nullptr, nullptr, GetHInstance(), nullptr);

	if (!_hWindow)
	{
		MessageBox(0, "Failed to create window", 0, 0);
		return;
	}

	SetWindowLongPtr(_hWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
	
  ShowCursor(FALSE);

  SetCapture(_hWindow);

	_captureMouse = true;
}

void Window::Show()
{
	ShowWindow(_hWindow, SW_SHOW);
}

HWND Window::GetHWND() {
	return _hWindow;
}

FLOAT Window::GetXChange()
{
	FLOAT changeValueX = _xChange;
	_xChange = 0.0f;
	return changeValueX;
}

FLOAT Window::GetYChange()
{
	FLOAT changeValueY = _yChange;
	_yChange = 0.0f;
	return changeValueY;
}

void Window::CleanUp()
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