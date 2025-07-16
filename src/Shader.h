#pragma once

#include "pch.h"

#include "D3D12Core.h"

enum class SHADERTYPE
{
	INVALID = -1,
	VERTEX = 0,
	PIXEL = 1,
	COMPUTE = 2,
};

class Shader
{
public:
	Shader() {};
	Shader(std::filesystem::path path, SHADERTYPE shaderType);

	SHADERTYPE _shaderType = SHADERTYPE::INVALID;
	MSWRL::ComPtr<IDxcResult> _compiledShaderBuffer;
	MSWRL::ComPtr<IDxcBlob> _shaderBlob;
};
