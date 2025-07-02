#pragma once

#include "precompiled/pch.h"

#include "Model.h"

class GLTFLoader
{
public:
	static void ConstructModelFromFile(std::filesystem::path path, Model& model, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

private:
	static std::vector<uint32_t> ExtractIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<uint32_t>& indices);
	static std::vector<Vertex> ExtractVertices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<Vertex>& vertices);

	static fastgltf::Parser _parser;
	static INT _modelIdIncrementor;
};