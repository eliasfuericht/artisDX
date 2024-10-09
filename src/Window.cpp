#include "Window.h"

LRESULT CALLBACK WindowProcess(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

Window::Window(const CHAR* title, INT w, INT h)
	: _windowTitle(title), _windowWidth(w), _windowHeight(h)
{}

int Window::Create()
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
		return NOTOK;
	}

	return OK;
}

void Window::Show()
{
	ShowWindow(_hWindow, SW_SHOW);
}

std::weak_ptr<HWND> Window::GetHWND() {
	return std::weak_ptr<HWND>((std::shared_ptr<HWND>(&_hWindow, [](HWND*) { /* no-op */ })));
}