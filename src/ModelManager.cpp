#include "ModelManager.h"

ModelManager::ModelManager(MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_device = device;
	_commandList = commandList;
	// FrustumCuller::GetInstance().CreateModelMatrixBuffer(device);
}

bool ModelManager::LoadModel(std::filesystem::path path)
{
	constexpr auto gltfOptions =
		fastgltf::Options::DontRequireValidAssetMember |
		fastgltf::Options::AllowDouble |
		fastgltf::Options::LoadExternalBuffers |
		fastgltf::Options::LoadExternalImages |
		fastgltf::Options::GenerateMeshIndices;

	auto data = fastgltf::MappedGltfFile::FromPath(path);
	if (!bool(data)) {
		std::cerr << "Failed to open glTF file: " << fastgltf::getErrorMessage(data.error()) << '\n';
		return false;
	}

	auto asset = _parser.loadGltf(data.get(), path.parent_path(), gltfOptions);
	if (auto error = asset.error(); error != fastgltf::Error::None) 
	{
		// Some error occurred while reading the buffer, parsing the JSON, or validating the data.
		std::cout << "Error occurred while parsing " << path << '\n';
		return false;
	}

	std::vector<std::vector<Vertex>> submeshVertices;
	std::vector<std::vector<uint32_t>> submeshIndices;
	std::vector<XMFLOAT4X4> submeshModelMatrices;
	std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures;

	for (auto& mesh : asset->meshes) 
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		XMFLOAT4X4 modelMatrix;

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

		// extract modelmatrix
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

		std::unordered_map<size_t, Texture::TEXTURETYPE> imageToType;

		for (size_t i = 0; i < asset->images.size(); ++i) {
			const fastgltf::Image& assetImage = asset->images[i];

			const uint8_t* pixelData = nullptr;
			size_t pixelSize = 0;

			int width = 0;
			int height = 0;
			int channels = 0;

			// Extract encoded image bytes (e.g. PNG, JPEG)
			if (auto bufferViewPtr = std::get_if<fastgltf::sources::BufferView>(&assetImage.data)) {
				const fastgltf::sources::BufferView& view = *bufferViewPtr;
				const auto& bufferViewMeta = asset->bufferViews[view.bufferViewIndex];
				const auto& buffer = asset->buffers[bufferViewMeta.bufferIndex];

				if (auto arrayPtr = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
					pixelData = reinterpret_cast<const uint8_t*>(arrayPtr->bytes.data()) + bufferViewMeta.byteOffset;
					pixelSize = bufferViewMeta.byteLength;
				}
			}

			if (!pixelData || pixelSize == 0)
				continue;

			ScratchImage image;
			TexMetadata metadata = {};
			metadata.width = width;
			metadata.height = height;
			metadata.mipLevels = 1;
			metadata.arraySize = 1;
			metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;
			metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;

			ThrowIfFailed(LoadFromWICMemory(pixelData, pixelSize, WIC_FLAGS_NONE, &metadata, image));

			Texture::TEXTURETYPE type = Texture::TEXTURETYPE::ALBEDO;
			if (auto it = imageToType.find(i); it != imageToType.end()) {
				type = it->second;
			}

			textures.emplace_back(type, std::move(image));
		}

		submeshVertices.push_back(vertices);
		submeshIndices.push_back(indices);
		submeshModelMatrices.push_back(modelMatrix);
	}

	std::shared_ptr<Model> model = std::make_shared<Model>(_modelId++, _device, _commandList, submeshVertices, submeshIndices, submeshModelMatrices, std::move(textures));
	model->RegisterWithGUI();
	_models.push_back(std::move(model)); 

	return true;
}

void ModelManager::DrawAll()
{
	// UpdateModels();

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