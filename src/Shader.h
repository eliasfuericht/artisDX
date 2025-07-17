#pragma once

#include "pch.h"

#include "D3D12Core.h"

enum SHADERTYPE : int32_t
{
	SHADER_INVALID = NOTOK,
	SHADER_VERTEX = 0,
	SHADER_PIXEL = 1,
	SHADER_COMPUTE = 2
};

class Shader
{
public:
	Shader() {};
	Shader(const std::filesystem::path&, SHADERTYPE shaderType);

	SHADERTYPE _shaderType = SHADER_INVALID;
	MSWRL::ComPtr<IDxcResult> _compiledShaderBuffer;
	MSWRL::ComPtr<IDxcBlob> _shaderBlob;
};
