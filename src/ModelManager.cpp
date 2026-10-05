#include "ModelManager.h"

void ModelManager::LoadModel(const std::filesystem::path& path)
{
	CommandContext uploadContext;
	uploadContext.InitializeCommandContext(QUEUETYPE::QUEUE_UPLOAD);

	std::shared_ptr<Model> model;

	bool constructedModel = GLTFLoader::ConstructModelFromFile(path, model, uploadContext.GetCommandList());

	if (constructedModel)
	{
		uploadContext.KeepAlive(model);
		model->RegisterWithGUI();

		_models.push_back(std::move(model));
	}

	uploadContext.Finish(true);
}

void ModelManager::DrawAll(const ShaderPass& shaderPass, CommandContext& commandContext)
{
	for (auto& model : _models)
	{
		commandContext.KeepAlive(model);
		model->DrawModel(shaderPass, commandContext.GetCommandList());
	}
}

void ModelManager::DrawAllBoundingBoxes(const ShaderPass& shaderPass, CommandContext& commandContext)
{
	for (auto& model : _models)
	{
		commandContext.KeepAlive(model);
		model->DrawModelBoundingBox(shaderPass, commandContext.GetCommandList());
	}
}
void ModelManager::ClearModels()
{
	// Recorded/submitted contexts retain their own model owners until completion.
	_models.clear();
}
