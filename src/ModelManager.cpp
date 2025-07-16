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

void ModelManager::DrawAll(ShaderPass& shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	for (auto& model : _models)
	{
		model->DrawModel(shaderPass, commandList);
	}
}