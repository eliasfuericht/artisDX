// pch - PRECOMPILED HEADER
#pragma once

// Windows
#include <windows.h> 
#include <wrl.h>
#include <filesystem>

// DirectX
#include <direct.h>
#include <d3d12.h>
#include <../build/_deps/d3dx12-src/include/directx/d3dx12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>

// STL
#include <memory>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <comdef.h> 
#include <sstream>
#include <numeric>
#include <tuple>

// GLM (not used directly but fastgltf depends on it - cant remove it)
#include <../build/_deps/glm-src/glm/glm.hpp>
#include <../build/_deps/glm-src/glm/gtc/matrix_transform.hpp>

// IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../build/_deps/imgui-src/imgui.h"
#include "../build/_deps/imgui-src/backends/imgui_impl_win32.h"

// fastgltf 
#include "../build/_deps/fastgltf-src/include/fastgltf/core.hpp"
#include "../build/_deps/fastgltf-src/include/fastgltf/types.hpp"
#include "../build/_deps/fastgltf-src/include/fastgltf/tools.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#include "../build/_deps/imgui-src/backends/imgui_impl_dx12.h"

using namespace DirectX;

// Defines
#define CHECK INT
#define OK 0
#define NOTOK -1

#define MSWRL Microsoft::WRL

#define GetHInstance() GetModuleHandle(NULL)

#define NUM_MAX_DESCRIPTORS 1024

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
		if (!errorMsg.empty())
		{
			fullError += errorMsg + " | ";
		}
		fullError += "HRESULT: " + std::to_string(hr) + " | Error Message: " + err.ErrorMessage() + " | ";

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

inline XMFLOAT4X4 ToXMFloat4x4(const fastgltf::math::fmat4x4& m)
{
	return XMFLOAT4X4(
		m[0][0], m[0][1], m[0][2], m[0][3],
		m[1][0], m[1][1], m[1][2], m[1][3],
		m[2][0], m[2][1], m[2][2], m[2][3],
		m[3][0], m[3][1], m[3][2], m[3][3]
	);
}

inline XMVECTOR XMQuaternionToRollPitchYaw(FXMVECTOR q) {
	float qx = XMVectorGetX(q);
	float qy = XMVectorGetY(q);
	float qz = XMVectorGetZ(q);
	float qw = XMVectorGetW(q);

	float sinr_cosp = 2 * (qw * qx + qy * qz);
	float cosr_cosp = 1 - 2 * (qx * qx + qy * qy);
	float roll = std::atan2(sinr_cosp, cosr_cosp);

	float sinp = 2 * (qw * qy - qz * qx);
	float pitch;
	if (std::abs(sinp) >= 1)
		pitch = std::copysign(XM_PIDIV2, sinp);
	else
		pitch = std::asin(sinp);

	float siny_cosp = 2 * (qw * qz + qx * qy);
	float cosy_cosp = 1 - 2 * (qy * qy + qz * qz);
	float yaw = std::atan2(siny_cosp, cosy_cosp);

	return XMVectorSet(yaw, pitch, roll, 0.0f);
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
	SHIFT = 16
};

struct Vertex {
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT4 tangent;
	XMFLOAT2 uv;
};