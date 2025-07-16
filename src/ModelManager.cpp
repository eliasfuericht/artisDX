#include "ModelManager.h"

void ModelManager::LoadModel(std::filesystem::path path)
{
	CommandContext uploadContext;
	uploadContext.InitializeCommandContext(QUEUETYPE::UPLOAD);

	std::shared_ptr<Model> model;

	_gltfLoader.ConstructModelFromFile(path, model, uploadContext.GetCommandList());

	model->RegisterWithGUI();

	_models.push_back(std::move(model));

	uploadContext.Finish(true);
}

void ModelManager::DrawAll(ShaderPass& shaderPass, CommandContext& commandContext)
{
	for (auto& model : _models)
	{
		model->DrawModel(shaderPass, commandContext.GetCommandList());
	}
}

void ModelManager::DrawAllBoundingBoxes(ShaderPass& shaderPass, CommandContext& commandContext)
{
	for (auto& model : _models)
	{
		model->DrawModelBoundingBox(shaderPass, commandContext.GetCommandList());
	}
}