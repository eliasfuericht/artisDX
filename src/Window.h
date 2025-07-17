#pragma once

#include "pch.h"

#include <hidusage.h>
#include "D3D12Core.h"

class Window
{
public:
	Window(const char* title, uint32_t w, uint32_t h, bool fullscreen);

	void Create();
	void Show();
	void HandleKeys(int32_t key, int32_t action);
	void HandleMouse(float x, float y);

	HWND GetHWND();
	uint32_t GetWidth() { return _windowWidth; };
	uint32_t GetHeight() { return _windowHeight; };

	void SetWidth(uint32_t width) { _windowWidth = width; };
	void SetHeight(uint32_t height) { _windowHeight = height; };

	bool* GetKeys() { return _keys; }
	float GetXChange();
	float GetYChange();
	void Shutdown();

	bool _captureMouse;
private:
	WNDCLASSEX _windowClassEx;
	const char* _windowTitle;
	ULONG _windowMode;
	uint32_t _windowWidth;
	uint32_t _windowHeight;
	HWND _hWindow;

	bool _keys[1024];
	float _xChange;
	float _yChange;
};