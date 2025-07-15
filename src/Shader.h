#pragma once

#include "pch.h"

#include "D3D12Core.h"

namespace D3D12Core::GraphicsDevice
{
	void InitializeShaderCompiler();

	extern MSWRL::ComPtr<IDxcUtils> _utils;
	extern MSWRL::ComPtr<IDxcCompiler3> _compiler;
	extern MSWRL::ComPtr<IDxcIncludeHandler> _includeHandler;
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
