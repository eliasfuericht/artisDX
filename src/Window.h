#pragma once

#include "pch.h"

#include <hidusage.h>
#include "D3D12Core.h"

namespace Window
{
	void InitializeWindow(const char* title, uint32_t w, uint32_t h, bool fullscreen);
	void Create();
	void Show();
	void HandleKeys(int32_t key, int32_t action);
	void HandleMouse(float x, float y);
	float GetXChange();
	float GetYChange();
	void Shutdown();

	extern WNDCLASSEX windowClassEx;
	extern const char* windowTitle;
	extern ULONG windowMode;
	extern uint32_t width;
	extern uint32_t height;
	extern HWND hWindow;
	extern bool initialized;

	extern bool captureMouse;
	extern bool keys[1024];
	extern float xChange;
	extern float yChange;
}