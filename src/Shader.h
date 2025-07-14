#pragma once

#include "pch.h"

#include "GraphicsDevice.h"

namespace D3D12Core::GraphicsDevice
{
	void InitializeShaderCompiler();

	extern Microsoft::WRL::ComPtr<IDxcUtils> _utils;
	extern Microsoft::WRL::ComPtr<IDxcCompiler3> _compiler;
	extern Microsoft::WRL::ComPtr<IDxcIncludeHandler> _includeHandler;
}

enum SHADERTYPE
{
	VERTEX,
	PIXEL,
	COMPUTE,
};

class Shader
{
public:
	Shader(std::filesystem::path path, SHADERTYPE shaderType);

private:
};
