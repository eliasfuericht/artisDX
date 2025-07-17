#pragma once

#include "pch.h"

#include "Model.h"

class GLTFLoader
{
public:
	static void ConstructModelFromFile(std::filesystem::path path, std::shared_ptr<Model>& model, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

private:
	static void ExtractIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<uint32_t>& indices);
	static void ExtractVertices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<Vertex>& vertices, bool& generateTangents);
	static void GenerateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	static void GenerateBiTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

	static ScratchImage ExtractImageFromBuffer(const fastgltf::Asset& asset, const fastgltf::Image& assetImage);

	static ScratchImage LoadFallbackAlbedoTexture();
	static ScratchImage LoadFallbackMetallicRoughnessTexture();
	static ScratchImage LoadFallbackNormalTexture();
	static ScratchImage LoadFallbackEmissiveTexture();
	static ScratchImage LoadFallbackOcclusionTexture();
	static ScratchImage Create1x1Texture(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

	static fastgltf::Parser _parser;
	static int32_t _modelIdIncrementor;
};