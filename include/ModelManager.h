#pragma once

#include "pch.h"

#include "Model.h"


class ModelManager
{
public:
	ModelManager();

	bool LoadModel(std::filesystem::path path);

private:
	fastgltf::Parser _parser;
	std::vector<Model> _models;

};