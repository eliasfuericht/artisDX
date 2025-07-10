#pragma once

#include "pch.h"

#include "GLTFLoader.h"
#include "Model.h"

class ModelManager
{
public:
	ModelManager() {};
	ModelManager(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	void LoadModel(std::filesystem::path path);
	void DrawAll();

private:
	MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;

	GLTFLoader _gltfLoader;
	std::vector<std::shared_ptr<Model>> _models;
};