#pragma once
#include <windows.h>
#include <memory>

#include "Defines.h"

class Window
{
public:
	Window(const CHAR* title, INT w, INT h);

	INT Create();
	void Show();
	std::weak_ptr<HWND> GetHWND();

private:
	WNDCLASSEX _windowClassEx;
	const CHAR* _windowTitle;
	INT _windowWidth;
	INT _windowHeight;
	HWND _hWindow;
};