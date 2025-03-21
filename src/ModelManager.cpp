#include "ModelManager.h"

ModelManager::ModelManager(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	// instantiate all the necessary gltf loaders and stuff
	_device = device;
	_commandList = commandList;
	Culler::GetInstance().CreateModelMatrixBuffer(device);
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
			const fastgltf::Accessor& indexAccessor = asset->accessors[primitive.indicesAccessor.value()];
			indices.reserve(indices.size() + indexAccessor.count);
			fastgltf::iterateAccessor<std::uint32_t>(asset.get(), indexAccessor, [&](std::uint32_t idx) {
				indices.push_back(idx);
				});

			const fastgltf::Accessor& positionAccessor = asset->accessors[primitive.findAttribute("POSITION")->accessorIndex];
			vertices.resize(vertices.size() + positionAccessor.count);

			fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset.get(), positionAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
				vertices[idx].position = XMFLOAT3(pos.x(), pos.y(), pos.z());
				});

			if (primitive.findAttribute("NORMAL") != primitive.attributes.end())
			{
				const fastgltf::Accessor& normalAccessor = asset->accessors[primitive.findAttribute("NORMAL")->accessorIndex];
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset.get(), normalAccessor, [&](fastgltf::math::fvec3 normal, std::size_t idx) {
					vertices[idx].normal = XMFLOAT3(normal.x(), normal.y(), normal.z());
					});
			}

			if (primitive.findAttribute("TANGENT") != primitive.attributes.end())
			{
				const fastgltf::Accessor& tangentAccessor = asset->accessors[primitive.findAttribute("TANGENT")->accessorIndex];

				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset.get(), tangentAccessor, [&](fastgltf::math::fvec4 tangent, std::size_t idx) {
					vertices[idx].tangent = XMFLOAT4(tangent.x(), tangent.y(), tangent.z(), tangent.w());
					});
			}

			if (primitive.findAttribute("TEXCOORD_0") != primitive.attributes.end())
			{
				const fastgltf::Accessor& uvAccessor = asset->accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];

				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset.get(), uvAccessor, [&](fastgltf::math::fvec2 uv, std::size_t idx) {
					vertices[idx].uv = XMFLOAT2(uv.x(), uv.y());
					});
			}
		}

		// transform vectors are in asset->nodes->transform
		XMFLOAT4X4 modelMatrix;
		XMStoreFloat4x4(&modelMatrix, XMMatrixIdentity());

		auto& transform = asset->nodes[0].transform;
		
		if (std::holds_alternative<fastgltf::TRS>(transform)) {
			const auto& trs = std::get<fastgltf::TRS>(transform);

			DirectX::XMVECTOR translation = DirectX::XMVectorSet(trs.translation[0], trs.translation[1], trs.translation[2], 1.0f);
			DirectX::XMVECTOR rotation = DirectX::XMVectorSet(trs.rotation[0], trs.rotation[1], trs.rotation[2], trs.rotation[3]);
			DirectX::XMVECTOR scale = DirectX::XMVectorSet(trs.scale[0], trs.scale[1], trs.scale[2], 1.0f);
			
			DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixScalingFromVector(scale) *
				DirectX::XMMatrixRotationQuaternion(rotation) *
				DirectX::XMMatrixTranslationFromVector(translation);

			XMStoreFloat4x4(&modelMatrix, transformMatrix);
		}

		// TODO: make hashes as id
		INT id = _models.size();
		std::shared_ptr<Model> model = std::make_shared<Model>(id, _device, vertices, indices, modelMatrix);
		model->RegisterWithGUI();
		_models.push_back(std::move(model));
	}

	return true;
}

void ModelManager::DrawAll()
{
	UpdateModels();

	for (auto& model : _models)
	{
		model->DrawModel(_commandList);
	}
}

void ModelManager::UpdateModels()
{
	for (auto& model : _models)
	{
		if (model->_markedForDeletion)
		{
			// TODO: Make model deletion possible
			//_models.erase(model->GetID());
		}
	}
}

void ModelManager::DrawAllCulled(XMFLOAT4X4 viewProjMatrix)
{
	UpdateModels();

	Culler::GetInstance().ExtractPlanes(viewProjMatrix, _device);
	Culler::GetInstance().BindMeshData(_commandList);

	for (auto& model : _models)
	{
		bool draw = Culler::GetInstance().CheckAABB(model->GetAABB(), model->GetModelMatrix());
		if (draw)
		{
			model->DrawModel(_commandList);
		}
		else
		{
			PRINT("Culled");
		}
	}

}

void ModelManager::TranslateModel(XMFLOAT3 vec, UINT modelId)
{
	_models[modelId]->Translate(vec);
}

void ModelManager::RotateModel(XMFLOAT3 vec, UINT modelId)
{
	_models[modelId]->Rotate(vec);
}

void ModelManager::ScaleModel(XMFLOAT3 vec, UINT modelId)
{
	_models[modelId]->Scale(vec);
}