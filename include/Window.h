#pragma once

#include "pch.h"

class Window
{
public:
	Window() {};
	Window(const CHAR* title, UINT w, UINT h);

	CHECK Create();
	CHECK Show();
	HWND GetHWND();
	UINT GetWidth() { return _windowWidth; };
	UINT GetHeight() { return _windowHeight; };

private:

	WNDCLASSEX _windowClassEx;
	const CHAR* _windowTitle;
	UINT _windowWidth;
	UINT _windowHeight;
	HWND _hWindow;
};