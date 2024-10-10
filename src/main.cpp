#pragma once
#include <stdio.h>

#include "Window.h"
#include "Renderer.h"

int main()
{
	Window window = Window("artisDX", 1280, 720);

	if (window.Create() != OK)
		return NOTOK;
		
	Renderer renderer(window);

	if (window.Show() != OK)
		return NOTOK;

	MSG msg = { 0 };

	while (msg.message != WM_QUIT)
	{
		renderer.Render();
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return OK;
}