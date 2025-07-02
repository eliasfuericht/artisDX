#include "ModelManager.h"

ModelManager::ModelManager(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_commandList = commandList;
}

void ModelManager::LoadModel(std::filesystem::path path)
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
		return;
	}

	auto asset = _parser.loadGltf(data.get(), path.parent_path(), gltfOptions);
	if (auto error = asset.error(); error != fastgltf::Error::None) 
	{
		// Some error occurred while reading the buffer, parsing the JSON, or validating the data.
		std::cout << "Error occurred while parsing " << path << '\n';
		return;
	}

	std::vector<std::vector<Vertex>> submeshVertices;
	std::vector<std::vector<uint32_t>> submeshIndices;
	std::vector<XMFLOAT4X4> submeshModelMatrices;
	std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures;
	std::vector<INT> materialIndices;
	std::vector<std::tuple<INT, std::vector<INT>>> materials;

	for (auto& mesh : asset->meshes) 
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		XMFLOAT4X4 modelMatrix;
		materialIndices.push_back(mesh.primitives[0].materialIndex.value_or(0));
		BOOL generateTangents = false;

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
			else
			{
				generateTangents = true;
			}

			if (primitive.findAttribute("TEXCOORD_0") != primitive.attributes.end())
			{
				const fastgltf::Accessor& uvAccessor = asset->accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];

				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset.get(), uvAccessor, [&](fastgltf::math::fvec2 uv, std::size_t idx) {
					vertices[idx].uv = XMFLOAT2(uv.x(), uv.y());
					});
			}
		}

		if (generateTangents)
		{
			GenerateTangents(vertices, indices);
		}

		// extract modelmatrix
		XMStoreFloat4x4(&modelMatrix, XMMatrixIdentity());

		auto& transform = asset->nodes[0].transform;
		
		if (std::holds_alternative<fastgltf::TRS>(transform)) {
			const auto& trs = std::get<fastgltf::TRS>(transform);

			DirectX::XMVECTOR translation = DirectX::XMVectorSet(trs.translation[0], trs.translation[1], trs.translation[2], 1.0f);
			DirectX::XMVECTOR rotation = DirectX::XMVectorSet(trs.rotation[0], trs.rotation[1], trs.rotation[2], trs.rotation[3]);
			DirectX::XMVECTOR scale = DirectX::XMVectorSet(trs.scale[0], trs.scale[1], trs.scale[2], 1.0f);
			
			DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixScalingFromVector(scale) * DirectX::XMMatrixRotationQuaternion(rotation) * DirectX::XMMatrixTranslationFromVector(translation);

			XMStoreFloat4x4(&modelMatrix, transformMatrix);
		}

		submeshVertices.push_back(vertices);
		submeshIndices.push_back(indices);
		submeshModelMatrices.push_back(modelMatrix);
	}

	std::unordered_map<size_t, Texture::TEXTURETYPE> imageToType;

	for (int i = 0; i < asset->materials.size(); i++) {
		const auto& material = asset->materials[i];
		std::vector<INT> materialTextureIndices;

		// 1. Base color (albedo)
		if (material.pbrData.baseColorTexture.has_value()) {
			auto textureIndex = material.pbrData.baseColorTexture->textureIndex;

			materialTextureIndices.push_back(textureIndex);
			
			const auto& texture = asset->textures[textureIndex];
			if (texture.imageIndex.has_value()) {
				imageToType[texture.imageIndex.value()] = Texture::TEXTURETYPE::ALBEDO;
			}
		}

		// 2. Metallic-Roughness
		if (material.pbrData.metallicRoughnessTexture.has_value()) {
			auto textureIndex = material.pbrData.metallicRoughnessTexture->textureIndex;

			materialTextureIndices.push_back(textureIndex);

			const auto& texture = asset->textures[textureIndex];
			if (texture.imageIndex.has_value()) {
				imageToType[texture.imageIndex.value()] = Texture::TEXTURETYPE::METALLICROUGHNESS;
			}
		}

		// 3. Normal map
		if (material.normalTexture.has_value()) {
			auto textureIndex = material.normalTexture->textureIndex;

			materialTextureIndices.push_back(textureIndex);

			const auto& texture = asset->textures[textureIndex];
			if (texture.imageIndex.has_value()) {
				imageToType[texture.imageIndex.value()] = Texture::TEXTURETYPE::NORMAL;
			}
		}

		// 4. Emissive
		if (material.emissiveTexture.has_value()) {
			auto textureIndex = material.emissiveTexture->textureIndex;

			materialTextureIndices.push_back(textureIndex);

			const auto& texture = asset->textures[textureIndex];
			if (texture.imageIndex.has_value()) {
				imageToType[texture.imageIndex.value()] = Texture::TEXTURETYPE::EMISSIVE;
			}
		}

		// 5. Occlusion
		if (material.occlusionTexture.has_value()) {
			auto textureIndex = material.occlusionTexture->textureIndex;

			materialTextureIndices.push_back(textureIndex);

			const auto& texture = asset->textures[textureIndex];
			if (texture.imageIndex.has_value()) {
				imageToType[texture.imageIndex.value()] = Texture::TEXTURETYPE::OCCLUSION;
			}
		}

		materials.emplace_back(std::tuple<INT, std::vector<INT>>(materialIndices[i], materialTextureIndices));
	}

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
			const auto& textureName = bufferViewMeta.name;
			const auto& buffer = asset->buffers[bufferViewMeta.bufferIndex];

			if (auto arrayPtr = std::get_if<fastgltf::sources::Array>(&buffer.data)) {
				pixelData = reinterpret_cast<const uint8_t*>(arrayPtr->bytes.data()) + bufferViewMeta.byteOffset;
				pixelSize = bufferViewMeta.byteLength;
			}
		}

		if (!pixelData || pixelSize == 0)
			continue;

		ScratchImage image;
		TexMetadata metadata;

		ThrowIfFailed(LoadFromWICMemory(pixelData, pixelSize, WIC_FLAGS_NONE, &metadata, image));

		Texture::TEXTURETYPE type = Texture::TEXTURETYPE::ALBEDO;
		if (auto it = imageToType.find(i); it != imageToType.end()) {
			type = it->second;
		}

		textures.emplace_back(type, std::move(image));
	}

	std::shared_ptr<Model> model = std::make_shared<Model>(_modelId++, _commandList, submeshVertices, submeshIndices, submeshModelMatrices, std::move(textures), materials);
	model->RegisterWithGUI();
	_models.push_back(std::move(model)); 
}

void ModelManager::DrawAll()
{
	for (auto& model : _models)
	{
		model->DrawModel(_commandList);
	}
}

void ModelManager::GenerateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
	std::vector<XMFLOAT3> accumulatedTan(vertices.size(), XMFLOAT3(0, 0, 0));
	std::vector<XMFLOAT3> accumulatedBitan(vertices.size(), XMFLOAT3(0, 0, 0));

	for (size_t i = 0; i < indices.size(); i += 3) {
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		const XMFLOAT3& p0 = vertices[i0].position;
		const XMFLOAT3& p1 = vertices[i1].position;
		const XMFLOAT3& p2 = vertices[i2].position;

		const XMFLOAT2& uv0 = vertices[i0].uv;
		const XMFLOAT2& uv1 = vertices[i1].uv;
		const XMFLOAT2& uv2 = vertices[i2].uv;

		XMVECTOR v0 = XMLoadFloat3(&p0);
		XMVECTOR v1 = XMLoadFloat3(&p1);
		XMVECTOR v2 = XMLoadFloat3(&p2);

		XMVECTOR edge1 = XMVectorSubtract(v1, v0);
		XMVECTOR edge2 = XMVectorSubtract(v2, v0);

		float du1 = uv1.x - uv0.x;
		float dv1 = uv1.y - uv0.y;
		float du2 = uv2.x - uv0.x;
		float dv2 = uv2.y - uv0.y;

		float det = du1 * dv2 - du2 * dv1;
		if (fabs(det) < 1e-6f) det = 1.0f;
		float invDet = 1.0f / det;

		XMVECTOR tangent = XMVectorScale(
			XMVectorSubtract(
				XMVectorScale(edge1, dv2),
				XMVectorScale(edge2, dv1)
			), invDet
		);

		XMVECTOR bitangent = XMVectorScale(
			XMVectorSubtract(
				XMVectorScale(edge2, du1),
				XMVectorScale(edge1, du2)
			), invDet
		);

		XMFLOAT3 t, b;
		XMStoreFloat3(&t, tangent);
		XMStoreFloat3(&b, bitangent);

		accumulatedTan[i0].x += t.x; accumulatedTan[i0].y += t.y; accumulatedTan[i0].z += t.z;
		accumulatedTan[i1].x += t.x; accumulatedTan[i1].y += t.y; accumulatedTan[i1].z += t.z;
		accumulatedTan[i2].x += t.x; accumulatedTan[i2].y += t.y; accumulatedTan[i2].z += t.z;

		accumulatedBitan[i0].x += b.x; accumulatedBitan[i0].y += b.y; accumulatedBitan[i0].z += b.z;
		accumulatedBitan[i1].x += b.x; accumulatedBitan[i1].y += b.y; accumulatedBitan[i1].z += b.z;
		accumulatedBitan[i2].x += b.x; accumulatedBitan[i2].y += b.y; accumulatedBitan[i2].z += b.z;
	}

	for (size_t i = 0; i < vertices.size(); ++i) {
		XMVECTOR N = XMVector3Normalize(XMLoadFloat3(&vertices[i].normal));
		XMVECTOR T = XMLoadFloat3(&accumulatedTan[i]);
		XMVECTOR B = XMLoadFloat3(&accumulatedBitan[i]);

		// Orthonormalize T with respect to N
		T = XMVector3Normalize(XMVectorSubtract(T, XMVectorScale(N, XMVector3Dot(N, T).m128_f32[0])));

		// Calculate handedness
		float handedness = (XMVector3Dot(XMVector3Cross(N, T), B).m128_f32[0] < 0.0f) ? -1.0f : 1.0f;

		XMFLOAT3 tangent;
		XMStoreFloat3(&tangent, T);
		vertices[i].tangent = XMFLOAT4(tangent.x, tangent.y, tangent.z, handedness);
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