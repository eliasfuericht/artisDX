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

	void LoadModel(const std::filesystem::path& path);
	void DrawAll(const ShaderPass& shaderPass, CommandContext& commandContext);
	void DrawAllBoundingBoxes(const ShaderPass& shaderPass, CommandContext& commandContext);

private:
	GLTFLoader _gltfLoader;
	std::vector<std::shared_ptr<Model>> _models;
};