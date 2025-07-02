#include "GLTFLoader.h"

fastgltf::Parser GLTFLoader::_parser;

void GLTFLoader::ConstructModelFromFile(std::filesystem::path path, Model& model)
{
	constexpr auto gltfOptions =
			fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble
		| fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers
		| fastgltf::Options::LoadExternalImages | fastgltf::Options::DecomposeNodeMatrices
		| fastgltf::Options::None;

	auto data = fastgltf::MappedGltfFile::FromPath(path);
	if (!bool(data)) {
		std::cerr << "Failed to open glTF file: " << fastgltf::getErrorMessage(data.error()) << '\n';
		return;
	}

	auto asset = _parser.loadGltf(data.get(), path.parent_path(), gltfOptions);
	if (auto error = asset.error(); error != fastgltf::Error::None)
	{
		// Some error occurred while reading the buffer, parsing the JSON, or validating the data.
		std::cout << "Error occurred while parsing " << path << '\n';
		return;
	}


}
/*
MeshInstance GLTFLoader::ExtractMeshSurface(const fastgltf::Asset& gltf, fastgltf::Primitive& primitive)
{
	return MeshInstance()
}*/

std::vector<uint32_t> GLTFLoader::ExtractIndices(const fastgltf::Asset& gltf, fastgltf::Primitive& primitive)
{
	return std::vector<uint32_t>();
}

std::vector<Vertex> GLTFLoader::ExtractVertices(const fastgltf::Asset& gltf, fastgltf::Primitive& primitive)
{
	return std::vector<Vertex>();
}
