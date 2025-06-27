#pragma once

#include "precompiled/pch.h"

#include "Model.h"

class ModelManager
{
public:
	ModelManager() {};
	ModelManager(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	bool LoadModel(std::filesystem::path path);
	void DrawAll();

	void TranslateModel(XMFLOAT3 vec, UINT modelId);
	void RotateModel(XMFLOAT3 vec, UINT modelId);
	void ScaleModel(XMFLOAT3 vec, UINT modelId);

private:
	MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
	fastgltf::Parser _parser;
	std::vector<std::shared_ptr<Model>> _models;
	INT _modelId = 0;
};