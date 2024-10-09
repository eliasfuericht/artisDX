#include <stdio.h>
#include <windows.h>
#include "Globals.h"

#define GetHInstance() GetModuleHandle(NULL)

int main()
{
	// Creating windowclass
	WNDCLASSEX windowClassEx;

	windowClassEx.cbSize = sizeof(WNDCLASSEX);
	windowClassEx.style = CS_HREDRAW | CS_VREDRAW;
	windowClassEx.cbClsExtra = 0;
	windowClassEx.cbWndExtra = 0;
	windowClassEx.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClassEx.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	windowClassEx.hIcon = LoadIcon(0, IDI_APPLICATION);
	windowClassEx.hIconSm = LoadIcon(0, IDI_APPLICATION);
	const char* windowClassName = "artisDXWindow";
	windowClassEx.lpszClassName = windowClassName;
	windowClassEx.lpszMenuName = nullptr;
	windowClassEx.hInstance = GetHInstance();
	windowClassEx.lpfnWndProc = DefWindowProc;

	RegisterClassEx(&windowClassEx);

	// Displaying window

	const char* windowName = "artisDX";

	const INT windowWidth = 1280;
	const INT windowHeight = 720;

	HWND hWindow = CreateWindow(windowClassName, windowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0,
															windowWidth, windowHeight, nullptr, nullptr, GetHInstance(), nullptr);

	if (!hWindow)
	{
		MessageBox(0, "Failed to create window", 0, 0);
		return 0;
	}

	ShowWindow(hWindow, SW_SHOW);

	MSG msg = { 0 };

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return 0;
}