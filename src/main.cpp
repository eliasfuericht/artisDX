#pragma once
#include <stdio.h>
#include <windows.h>

#include "Window.h"

int main()
{
	Window window("artisDX", 1280, 720);

	if (window.Create() == OK)
		window.Show();

	std::weak_ptr<HWND> hWindow = window.GetHWND();

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