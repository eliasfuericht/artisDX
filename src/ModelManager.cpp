#include "ModelManager.h"

ModelManager::ModelManager(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_commandList = commandList;
}

void ModelManager::LoadModel(std::filesystem::path path)
{
	std::shared_ptr<Model> model;

	_gltfLoader.ConstructModelFromFile(path, model, _commandList);

	model->RegisterWithGUI();

	_models.push_back(std::move(model));
}

void ModelManager::DrawAll()
{
	for (auto& model : _models)
	{
		model->DrawModel(_commandList);
	}
}