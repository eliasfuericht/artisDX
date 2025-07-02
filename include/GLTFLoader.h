#pragma once

#include "precompiled/pch.h"

#include "Model.h"

class GLTFLoader
{
public:
	static void ConstructModelFromFile(std::filesystem::path path, Model& model);

private:
	static std::vector<uint32_t> ExtractIndices(const fastgltf::Asset& gltf, fastgltf::Primitive& primitive);
	static std::vector<Vertex> ExtractVertices(const fastgltf::Asset& gltf, fastgltf::Primitive& primitive);
	//static MeshInstance ExtractMeshSurface(const fastgltf::Asset& gltf, fastgltf::Primitive& primitive);

	static fastgltf::Parser _parser;
};