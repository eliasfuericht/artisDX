#pragma once

#include "pch.h"
#include "Model.h"

#include "CommandQueue.h"
#include "CommandContext.h"

class ShaderPass;

class ModelManager
{
public:
	ModelManager() = default;

	void ClearModels();
	void LoadModel(const std::filesystem::path& path);
	void ComputeGlobalTransforms();
	void DrawAll(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext, const XMFLOAT4X4& viewMatrix);
	void DrawAllBoundingBoxes(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext);

private:
	std::vector<std::shared_ptr<Model>> _models;
	std::vector<Model::DrawItem> _drawItems;
};
