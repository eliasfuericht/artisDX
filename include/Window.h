#pragma once

#include "pch.h"

class Window
{
public:
	Window(const CHAR* title, UINT w, UINT h);

	CHECK Create();
	CHECK Show();
	std::weak_ptr<HWND> GetHWND();
	UINT GetWidth() { return _windowWidth; };
	UINT GetHeight() { return _windowHeight; };

private:
	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	WNDCLASSEX _windowClassEx;
	const CHAR* _windowTitle;
	UINT _windowWidth;
	UINT _windowHeight;
	HWND _hWindow;
};