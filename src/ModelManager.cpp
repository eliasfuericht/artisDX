#include "ModelManager.h"

ModelManager::ModelManager()
{
	// instantiate all the necessary gltf loaders and stuff

}

bool ModelManager::LoadModel(std::filesystem::path path)
{
	auto data = fastgltf::GltfDataBuffer::FromPath(path);
	if (data.error() != fastgltf::Error::None) {
		// The file couldn't be loaded, or the buffer could not be allocated.
		std::cout << "Failed to find " << path << '\n';
		return false;
	}

	auto asset = _parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::None);
	if (auto error = asset.error(); error != fastgltf::Error::None) {
		// Some error occurred while reading the buffer, parsing the JSON, or validating the data.
		std::cout << "Error occurred while parsing " << path << '\n';
		return false;
	}

	for (auto& accessor : asset->accessors) {
		//TODO: extract vertex and index data
		//auto temp = accessor.;
	}

	return true;
	
	// load model and push inside vector
}