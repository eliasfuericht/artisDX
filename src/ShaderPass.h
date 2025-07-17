#pragma once

#include "pch.h"
#include <map>
#include <optional>

#include "GUI.h"
#include "IGUIComponent.h"
#include "Shader.h"

class ShaderPass : public IGUIComponent
{
public:
	ShaderPass() {};
	ShaderPass(const std::string& name);

	void AddShader(const std::filesystem::path& path, SHADERTYPE shaderType);

	void GenerateGraphicsRootSignature();
	void GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE fillMode, D3D12_CULL_MODE cullMode, bool alphaBlending);

	std::optional<uint32_t> GetRootParameterIndex(const std::string& name) const;

	void DrawGUI();

	MSWRL::ComPtr<ID3D12RootSignature> _rootSignature;
	MSWRL::ComPtr<ID3D12PipelineState> _pipelineState;

	std::unordered_map<std::string, uint32_t> _bindingMap;
	std::unordered_map<SHADERTYPE, Shader> _shaders;

	std::string _name;

	bool _usePass = true;
};