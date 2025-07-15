#pragma once

#include "pch.h"

#include <hidusage.h>
#include "D3D12Core.h"

class Window
{
public:
	Window(const CHAR* title, UINT w, UINT h, bool fullscreen);

	void Create();
	void Show();
	void HandleKeys(INT key, INT action);
	void HandleMouse(FLOAT x, FLOAT y);

	HWND GetHWND();
	UINT GetWidth() { return _windowWidth; };
	UINT GetHeight() { return _windowHeight; };

	void SetWidth(INT width) { _windowWidth = width; };
	void SetHeight(INT height) { _windowHeight = height; };

	BOOL* GetKeys() { return _keys; }
	FLOAT GetXChange();
	FLOAT GetYChange();
	void Shutdown();

	BOOL _captureMouse;
private:
	WNDCLASSEX _windowClassEx;
	const CHAR* _windowTitle;
	ULONG _windowMode;
	UINT _windowWidth;
	UINT _windowHeight;
	HWND _hWindow;

	BOOL _keys[1024];
	FLOAT _xChange;
	FLOAT _yChange;
};