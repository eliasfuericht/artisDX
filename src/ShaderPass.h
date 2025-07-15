#pragma once

#include "pch.h"
#include <map>
#include <optional>

#include "Shader.h"

class ShaderPass
{
public:
	ShaderPass() {};

	void AddShader(std::filesystem::path path, SHADERTYPE shaderType);

	void GenerateGraphicsRootSignature();
	void GeneratePipeLineStateObject();

	std::optional<UINT> GetRootParameterIndex(const std::string& name);

	MSWRL::ComPtr<ID3D12RootSignature> _rootSignature;
	MSWRL::ComPtr<ID3D12PipelineState> _pipelineState;

	std::unordered_map<std::string, uint32_t> _bindingMap;

private:
	std::unordered_map<SHADERTYPE, Shader> _shaders;
};