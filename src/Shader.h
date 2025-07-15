#pragma once

#include "pch.h"

#include "D3D12Core.h"

enum SHADERTYPE
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

	SHADERTYPE _shaderType = INVALID;
	MSWRL::ComPtr<IDxcResult> _compiledShaderBuffer;
	MSWRL::ComPtr<IDxcBlob> _shaderBlob;
	D3D12_SHADER_BYTECODE _shaderByteCode;
};
