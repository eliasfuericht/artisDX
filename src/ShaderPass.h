#pragma once

#include "pch.h"
#include <map>

#include "Shader.h"

class ShaderPass
{
public:
	ShaderPass() {};

	void AddShader(std::filesystem::path path, SHADERTYPE shaderType);

	void GenerateGraphicsRootSignature();
	void GeneratePipeLineStateObject();

	MSWRL::ComPtr<ID3D12RootSignature> _rootSignature;
	MSWRL::ComPtr<ID3D12PipelineState> _pipelineState;

private:
	std::unordered_map<SHADERTYPE, Shader> _shaders;
};