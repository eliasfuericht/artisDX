#pragma once

#include "pch.h"

class Window
{
public:
	Window(const CHAR* title, UINT w, UINT h);

	void Create();
	void Show();
	void HandleKeys(INT key, INT action);
	void HandleMouse(FLOAT x, FLOAT y);

	HWND GetHWND();
	UINT GetWidth() { return _windowWidth; };
	UINT GetHeight() { return _windowHeight; };

	BOOL* GetKeys() { return _keys; }
	FLOAT GetXChange();
	FLOAT GetYChange();
	void CleanUp();

	FLOAT _lastX;
	FLOAT _lastY;
private:
	// Win32
	WNDCLASSEX _windowClassEx;
	const CHAR* _windowTitle;
	UINT _windowWidth;
	UINT _windowHeight;
	HWND _hWindow;

	BOOL _keys[1024];
	FLOAT _xChange;
	FLOAT _yChange;
};