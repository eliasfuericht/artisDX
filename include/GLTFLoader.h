#pragma once

#include "precompiled/pch.h"

#include "Model.h"

class GLTFLoader
{
public:
	static void ConstructModelFromFile(std::filesystem::path path, std::shared_ptr<Model>& model, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

private:
	static void ExtractIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<uint32_t>& indices);
	static void ExtractVertices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<Vertex>& vertices, bool& generateTangents);
	static void GenerateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

	static ScratchImage ExtractImageFromBuffer(const fastgltf::Asset& asset, const fastgltf::Image& assetImage);

	static fastgltf::Parser _parser;
	static INT _modelIdIncrementor;
};