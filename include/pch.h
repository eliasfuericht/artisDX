// pch - PRECOMPILED HEADER
#pragma once

// Windows
#include <windows.h>

// DirectX
#include <direct.h>
#include <D3D12.h>
#include <D3Dcompiler.h>
#include <dxgi1_4.h>
#include <memory>
#include <algorithm>

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