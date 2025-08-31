#include "ModelManager.h"

void ModelManager::LoadModel(const std::filesystem::path& path)
{
	CommandContext uploadContext;
	uploadContext.InitializeCommandContext(QUEUETYPE::QUEUE_UPLOAD);

	std::shared_ptr<Model> model;

	bool constructedModel = GLTFLoader::ConstructModelFromFile(path, model, uploadContext.GetCommandList());

	if (constructedModel)
	{
		model->RegisterWithGUI();

		_models.push_back(std::move(model));
	}

	uploadContext.Finish(true);
}

void ModelManager::DrawAll(const ShaderPass& shaderPass, CommandContext& commandContext)
{
	for (auto& model : _models)
	{
		model->DrawModel(shaderPass, commandContext.GetCommandList());
	}
}

void ModelManager::DrawAllBoundingBoxes(const ShaderPass& shaderPass, CommandContext& commandContext)
{
	for (auto& model : _models)
	{
		model->DrawModelBoundingBox(shaderPass, commandContext.GetCommandList());
	}
}