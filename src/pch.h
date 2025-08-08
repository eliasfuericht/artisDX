// pch - PRECOMPILED HEADER
#pragma once

// STL
#include <string>
#include <list>
#include <vector>
#include <memory>
#include <tuple>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <chrono>
#include <sstream>
#include <numeric>
#include <filesystem>

// Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wrl.h>
#include <comdef.h>

// DirectX core
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <d3d12shader.h>
#include <DirectXMath.h>
#include <dxcapi.h>
#include <../build/_deps/d3dx12-src/include/directx/d3dx12.h>

// GLM (not used directly but fastgltf depends on it - cant remove it)
#include <../build/_deps/glm-src/glm/glm.hpp>
#include <../build/_deps/glm-src/glm/gtc/matrix_transform.hpp>

// IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include <../build/_deps/imgui-src/imgui.h>
#include <../build/_deps/imgui-src/backends/imgui_impl_win32.h>

// fastgltf 
#include <../build/_deps/fastgltf-src/include/fastgltf/core.hpp>
#include <../build/_deps/fastgltf-src/include/fastgltf/types.hpp>
#include <../build/_deps/fastgltf-src/include/fastgltf/tools.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

#include <../build/_deps/imgui-src/backends/imgui_impl_dx12.h>

using namespace DirectX;

// Defines
#define NOTOK -1

#define MSWRL Microsoft::WRL

#define GetHInstance() GetModuleHandle(NULL)

#define NUM_MAX_RESOURCE_DESCRIPTORS 65536
#define NUM_MAX_RTV_DESCRIPTORS 1024
#define NUM_MAX_DSV_DESCRIPTORS 1024
#define NUM_MAX_SAMPLER_DESCRIPTORS 512

template<typename... Args>
inline void PrintHelper(Args&&... args) {
	std::ostringstream oss;
	(oss << ... << args);
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

enum KEYCODES : uint32_t
{
	KEYCODE_W = 87,
	KEYCODE_A = 65,
	KEYCODE_S = 83,
	KEYCODE_D = 68,
	KEYCODE_SPACE = 32,
	KEYCODE_LCTRL = 17,
	KEYCODE_ESC = 27,
	KEYCODE_SHIFT = 16
};

struct Vertex {
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 uv;
	XMFLOAT4 tangent;
	XMFLOAT3 bitangent;
};

namespace Utils
{
	namespace Timer
	{
		inline std::chrono::high_resolution_clock::time_point& start_time()
		{
			static auto start = std::chrono::high_resolution_clock::now();
			return start;
		}

		inline void StartTimer()
		{
			start_time() = std::chrono::high_resolution_clock::now();
		}

		inline void PrintElapsedSeconds()
		{
			PRINT(std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time()).count(), "s");
		}

		inline void PrintElapsedMilliseconds()
		{
			PRINT(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time()).count(), "ms");
		}

		inline double GetElapsedSeconds()
		{
			return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time()).count();
		}

		inline double GetElapsedMilliseconds()
		{
			return std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - start_time()).count();
		}
	}

	inline XMFLOAT2 ToXMFloat2(const fastgltf::math::nvec2& v)
	{
		return XMFLOAT2(v[0], v[1]);
	}

	inline XMFLOAT3 ToXMFloat3(const fastgltf::math::nvec3& v)
	{
		return XMFLOAT3(v[0], v[1], v[2]);
	}

	inline XMFLOAT4 ToXMFloat4(const fastgltf::math::nvec4& v)
	{
		return XMFLOAT4(v[0], v[1], v[2], v[3]);
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
}