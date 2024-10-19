// pch - PRECOMPILED HEADER
#pragma once

// Windows
#include <windows.h> 
#include <wrl.h>

// DirectX
#include <direct.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>

// STL
#include <memory>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>

// Defines
#define CHECK INT
#define OK 0
#define NOTOK -1

#define MS Microsoft::WRL

#define GetHInstance() GetModuleHandle(NULL)

#define PRINT(arg) std::cout << arg << std::endl

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

enum KEYCODES
{
	W = 87,
	A = 65,
	S = 83,
	D = 68,
	SPACE = 32,
	LCTRL = 17,
	ESC = 27,
};