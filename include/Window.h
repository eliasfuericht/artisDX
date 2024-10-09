#pragma once

#include "pch.h"

class Window
{
public:
	Window(const CHAR* title, INT w, INT h);

	INT Create();
	void Show();
	std::weak_ptr<HWND> GetHWND();

private:
	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	WNDCLASSEX _windowClassEx;
	const CHAR* _windowTitle;
	INT _windowWidth;
	INT _windowHeight;
	HWND _hWindow;
};