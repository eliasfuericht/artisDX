// pch - PRECOMPILED HEADER
#pragma once

// Windows
#include <windows.h>

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

#define GetHInstance() GetModuleHandle(NULL)

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}