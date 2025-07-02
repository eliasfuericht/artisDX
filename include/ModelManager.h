#pragma once

#include "precompiled/pch.h"

#include "GLTFLoader.h"
#include "Model.h"

class ModelManager
{
public:
	ModelManager() {};
	ModelManager(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	void LoadModel(std::filesystem::path path);
	void DrawAll();

	void GenerateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

	void TranslateModel(XMFLOAT3 vec, UINT modelId);
	void RotateModel(XMFLOAT3 vec, UINT modelId);
	void ScaleModel(XMFLOAT3 vec, UINT modelId);

private:
	MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;

	GLTFLoader _gltfLoader;
	fastgltf::Parser _parser;
	std::vector<std::shared_ptr<Model>> _models;
	INT _modelId = 0;
};