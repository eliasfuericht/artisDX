#pragma once

#include "pch.h"

#include "D3D12Core.h"

#define NOTOK -1

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
	Shader(std::filesystem::path path, SHADERTYPE shaderType);

	SHADERTYPE _shaderType = INVALID;
	MSWRL::ComPtr<IDxcBlob> _shaderBlob;
	D3D12_SHADER_BYTECODE _shaderByteCode;
	MSWRL::ComPtr<ID3D12RootSignature> _rootSignature;
};
