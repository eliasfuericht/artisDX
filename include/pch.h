// pch - PRECOMPILED HEADER
#pragma once

// Windows
#include <windows.h> 
#include <wrl.h>
#include <filesystem>

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
#include <comdef.h>  // For _com_error
#include <sstream>

// GLM
#include <../build/_deps/glm-src/glm/glm.hpp>
#include <../build/_deps/glm-src/glm/gtc/matrix_transform.hpp>

// IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../build/_deps/imgui-src/imgui.h"
#include "../build/_deps/imgui-src/backends/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "../build/_deps/imgui-src/backends/imgui_impl_dx12.h"

// Defines
#define CHECK INT
#define OK 0
#define NOTOK -1

#define MSWRL Microsoft::WRL

#define GetHInstance() GetModuleHandle(NULL)

template<typename... Args>
void PrintHelper(Args&&... args) {
	std::ostringstream oss;
	(oss << ... << args);  // C++17 fold expression
	oss << "\n";
	OutputDebugStringA(oss.str().c_str());
}

#define PRINT(...) PrintHelper(__VA_ARGS__)


inline void ThrowIfFailed(HRESULT hr, const std::string& errorMsg = "")
{
	if (FAILED(hr))
	{
		_com_error err(hr);  // Convert HRESULT to readable error message
		std::string fullError;
		errorMsg == "" ? fullError = "" : fullError = "Error: ";
		if (!errorMsg.empty())
		{
			fullError += errorMsg + " | ";
		}
		fullError += "HRESULT: " + std::to_string(hr) + " | Error Message: " + err.ErrorMessage();

		OutputDebugStringA(fullError.c_str());
		throw std::runtime_error(fullError);
	}
}

inline void ThrowException(const std::string& errorMsg = "")
{
	std::string fullError;
	errorMsg == "" ? fullError = "" : fullError = "Error: ";
	if (!errorMsg.empty())
	{
		fullError += errorMsg + " | ";
	}

	OutputDebugStringA(fullError.c_str());
	throw std::runtime_error(fullError);
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