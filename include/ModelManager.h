#pragma once

#include "pch.h"

#include "Model.h"
#include "Culler.h"


class ModelManager
{
public:
	ModelManager() {};
	ModelManager(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	bool LoadModel(std::filesystem::path path);
	void DrawAll();
	void DrawAllCulled(XMFLOAT4X4 viewProjMatrix);

	INT CopyModel(INT id);

	void TranslateModel(XMFLOAT3 vec, UINT modelId);
	void RotateModel(XMFLOAT3 vec, UINT modelId);
	void ScaleModel(XMFLOAT3 vec, UINT modelId);

private:
	MSWRL::ComPtr<ID3D12Device> _device;
	MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
	fastgltf::Parser _parser;
	std::vector<Model*> _models;

};