#pragma once

#include "pch.h"

#include "GLTFLoader.h"
#include "Model.h"
#include "CommandQueue.h"
#include "CommandContext.h"

class ModelManager
{
public:
	ModelManager() {};

	void LoadModel(std::filesystem::path path);
	void DrawAll(ShaderPass& shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

private:
	GLTFLoader _gltfLoader;
	std::vector<std::shared_ptr<Model>> _models;
};