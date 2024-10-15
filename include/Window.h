#pragma once

#include "pch.h"

class Window
{
public:
	Window() {};
	Window(const CHAR* title, UINT w, UINT h);

	void Create();
	void Show();
	HWND GetHWND();
	UINT GetWidth() { return _windowWidth; };
	UINT GetHeight() { return _windowHeight; };

	void CleanUp();

private:
	// Win32
	WNDCLASSEX _windowClassEx;
	const CHAR* _windowTitle;
	UINT _windowWidth;
	UINT _windowHeight;
	HWND _hWindow;
};