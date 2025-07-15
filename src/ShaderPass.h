#pragma once

#include "pch.h"
#include <map>

#include "Shader.h"

class ShaderPass
{
public:
	ShaderPass() {};

	void AddShader(std::filesystem::path path, SHADERTYPE shaderType);

	void GenerateRootSignature();

	MSWRL::ComPtr<ID3D12RootSignature> _rootSignature;

private:
	std::unordered_map<SHADERTYPE, Shader> _shaders;
};