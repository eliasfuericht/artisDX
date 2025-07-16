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
	void GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE fillMode, D3D12_CULL_MODE cullMode, BOOL alphaBlending);
	void GeneratePipeLineStateObjectDepthPass();

	std::optional<UINT> GetRootParameterIndex(const std::string& name);

	MSWRL::ComPtr<ID3D12RootSignature> _rootSignature;
	MSWRL::ComPtr<ID3D12PipelineState> _pipelineStateFill;
	MSWRL::ComPtr<ID3D12PipelineState> _pipelineStateWireframe;

	std::unordered_map<std::string, uint32_t> _bindingMap;

private:
	std::unordered_map<SHADERTYPE, Shader> _shaders;
};