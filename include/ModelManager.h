#pragma once

#include "pch.h"

#include "Model.h"


class ModelManager
{
public:
	ModelManager() {};
	ModelManager(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	bool LoadModel(std::filesystem::path path);
	void DrawAllModels();

private:
	MSWRL::ComPtr<ID3D12Device> _device;
	MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
	fastgltf::Parser _parser;
	std::vector<Model> _models;

};