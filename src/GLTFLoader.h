#pragma once

#include "pch.h"

#include "Model.h"

namespace GLTFLoader
{
	bool ConstructModelFromFile(const std::filesystem::path& path, std::shared_ptr<Model>& model, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	void ExtractIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<uint32_t>& indices);
	void ExtractVertices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<Vertex>& vertices, bool& generateTangents);
	void GenerateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
	void GenerateBiTangents(std::vector<Vertex>& vertices);

	ScratchImage ExtractImageFromBuffer(const fastgltf::Asset& asset, const fastgltf::Image& assetImage);

	ScratchImage LoadFallbackAlbedoTexture();
	ScratchImage LoadFallbackMetallicRoughnessTexture();
	ScratchImage LoadFallbackNormalTexture();
	ScratchImage LoadFallbackEmissiveTexture();
	ScratchImage LoadFallbackOcclusionTexture();
	ScratchImage Create1x1Texture(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

	extern fastgltf::Parser parser;
	extern int32_t modelIdIncrementor;
}
