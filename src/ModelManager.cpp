#include "ModelManager.h"

ModelManager::ModelManager(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// instantiate all the necessary gltf loaders and stuff
	_device = device;
	_commandList = commandList;
}

bool ModelManager::LoadModel(std::filesystem::path path)
{
	auto data = fastgltf::GltfDataBuffer::FromPath(path);
	if (data.error() != fastgltf::Error::None) 
	{
		// The file couldn't be loaded, or the buffer could not be allocated.
		std::cout << "Failed to find " << path << '\n';
		return false;
	}

	auto asset = _parser.loadGltf(data.get(), path.parent_path(), fastgltf::Options::None);
	if (auto error = asset.error(); error != fastgltf::Error::None) 
	{
		// Some error occurred while reading the buffer, parsing the JSON, or validating the data.
		std::cout << "Error occurred while parsing " << path << '\n';
		return false;
	}

	for (auto& mesh : asset->meshes) 
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		for (auto& primitive : mesh.primitives) 
		{
			// indices
			const fastgltf::Accessor& indexAccessor = asset->accessors[primitive.indicesAccessor.value()];
			indices.reserve(indices.size() + indexAccessor.count);
			// Extract the actual asset
			fastgltf::iterateAccessor<std::uint32_t>(asset.get(), indexAccessor, [&](std::uint32_t idx) {
				indices.push_back(idx);
				});

			// vertices
			// position
			const fastgltf::Accessor& positionAccessor = asset->accessors[primitive.findAttribute("POSITION")->accessorIndex];
			vertices.resize(vertices.size() + positionAccessor.count);

			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset.get(), positionAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
				vertices[idx].position = DirectX::XMFLOAT3(pos.x(), pos.y(), pos.z());
				});

			// normals
			if (primitive.findAttribute("NORMAL") != primitive.attributes.end())
			{
				const fastgltf::Accessor& normalAccessor = asset->accessors[primitive.findAttribute("NORMAL")->accessorIndex];
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset.get(), normalAccessor, [&](fastgltf::math::fvec3 normal, std::size_t idx) {
					vertices[idx].normal = DirectX::XMFLOAT3(normal.x(), normal.y(), normal.z());
					});
			}

			// tangents
			if (primitive.findAttribute("TANGENT") != primitive.attributes.end())
			{
				const fastgltf::Accessor& tangentAccessor = asset->accessors[primitive.findAttribute("TANGENT")->accessorIndex];

				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset.get(), tangentAccessor, [&](fastgltf::math::fvec4 tangent, std::size_t idx) {
					vertices[idx].tangent = DirectX::XMFLOAT4(tangent.x(), tangent.y(), tangent.z(), tangent.w());
					});
			}

			if (primitive.findAttribute("TEXCOORD_0") != primitive.attributes.end())
			{
				const fastgltf::Accessor& uvAccessor = asset->accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];

				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset.get(), uvAccessor, [&](fastgltf::math::fvec2 uv, std::size_t idx) {
					vertices[idx].uv = DirectX::XMFLOAT2(uv.x(), uv.y());
					});
			}
		}

		DirectX::XMFLOAT4X4 modelMatrix;
		DirectX::XMStoreFloat4x4(&modelMatrix, DirectX::XMMatrixIdentity());

		Model model = Model(_device, vertices, indices, modelMatrix);
		_models.push_back(model);
	}

	return true;
}

void ModelManager::DrawAllModels()
{
	for (auto& model : _models)
	{
		model.DrawModel(_commandList);
	}
}

void ModelManager::TranslateModel(DirectX::XMFLOAT3 vec, UINT modelId)
{
	_models[modelId].Translate(vec);
}

void ModelManager::RotateModel(DirectX::XMFLOAT3 vec, UINT modelId)
{
	_models[modelId].Rotate(vec);
}

void ModelManager::ScaleModel(DirectX::XMFLOAT3 vec, UINT modelId)
{
	_models[modelId].Scale(vec);
}