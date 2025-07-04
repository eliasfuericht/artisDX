#include "GLTFLoader.h"

fastgltf::Parser GLTFLoader::_parser;
INT GLTFLoader::_modelIdIncrementor = 0;

void GLTFLoader::ConstructModelFromFile(std::filesystem::path path, std::shared_ptr<Model>& model, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
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

	std::string modelName = path.filename().string();

	// Extract Vertex and Index Information
	std::vector<Mesh> meshes;
	INT meshIdIncrementor = 0;
	for (const fastgltf::Mesh& mesh : asset->meshes)
	{
		std::vector<Primitive> primitives;

		for (const fastgltf::Primitive& primitive : mesh.primitives)
		{
			std::vector<uint32_t> indices;
			ExtractIndices(asset.get(), primitive, indices);
			std::vector<Vertex> vertices;
			bool generateTangents = false;
			ExtractVertices(asset.get(), primitive, vertices, generateTangents);

			if (generateTangents)
				GenerateTangents(vertices, indices);

			INT materialIndex = primitive.materialIndex.value();
			primitives.emplace_back(Primitive(vertices, indices, materialIndex));
		}

		meshes.emplace_back(Mesh(meshIdIncrementor++, primitives));
	}

	// extract materials and textures
	std::vector<Material> materials;
	std::vector<Texture> textures;
	INT textureIndexIncrementor = 0;
	for (const fastgltf::Material& gltfMaterial : asset->materials) 
	{
		Material material;

		// 1. BaseColor (Albedo)
		material._baseColorTextureIndex = textureIndexIncrementor++;
		if (gltfMaterial.pbrData.baseColorTexture.has_value()) 
		{
			size_t textureIndex = gltfMaterial.pbrData.baseColorTexture->textureIndex;

			const fastgltf::Texture& assetTexture = asset->textures[textureIndex];
			size_t imageIndex = assetTexture.imageIndex.value();
			const fastgltf::Image& assetImage = asset->images[imageIndex];

			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::ALBEDO;
			ScratchImage scratchImage = ExtractImageFromBuffer(asset.get(), assetImage);
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}
		else
		{
			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::ALBEDO;
			ScratchImage scratchImage = LoadFallbackAlbedoTexture();
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}

		// 2. Metallic-Roughness
		material._metallicRoughnessTextureIndex = textureIndexIncrementor++;
		if (gltfMaterial.pbrData.metallicRoughnessTexture.has_value()) 
		{
			size_t textureIndex = gltfMaterial.pbrData.metallicRoughnessTexture->textureIndex;

			const fastgltf::Texture& assetTexture = asset->textures[textureIndex];
			size_t imageIndex = assetTexture.imageIndex.value();
			const fastgltf::Image& assetImage = asset->images[imageIndex];

			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::METALLICROUGHNESS;
			ScratchImage scratchImage = ExtractImageFromBuffer(asset.get(), assetImage);
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}
		else
		{
			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::METALLICROUGHNESS;
			ScratchImage scratchImage = LoadFallbackMetallicRoughnessTexture();
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}

		// 3. Normal
		material._normalTextureIndex = textureIndexIncrementor++;
		if (gltfMaterial.normalTexture.has_value()) 
		{
			size_t textureIndex = gltfMaterial.normalTexture->textureIndex;

			const fastgltf::Texture& assetTexture = asset->textures[textureIndex];
			size_t imageIndex = assetTexture.imageIndex.value();
			const fastgltf::Image& assetImage = asset->images[imageIndex];

			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::NORMAL;
			ScratchImage scratchImage = ExtractImageFromBuffer(asset.get(), assetImage);
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}
		else
		{
			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::NORMAL;
			ScratchImage scratchImage = LoadFallbackNormalTexture();
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}

		// 4. Emissive
		material._emissiveTextureIndex = textureIndexIncrementor++;
		if (gltfMaterial.emissiveTexture.has_value()) 
		{
			size_t textureIndex = gltfMaterial.emissiveTexture->textureIndex;

			const fastgltf::Texture& assetTexture = asset->textures[textureIndex];
			size_t imageIndex = assetTexture.imageIndex.value();
			const fastgltf::Image& assetImage = asset->images[imageIndex];

			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::EMISSIVE;
			ScratchImage scratchImage = ExtractImageFromBuffer(asset.get(), assetImage);
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}
		else
		{
			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::EMISSIVE;
			ScratchImage scratchImage = LoadFallbackEmissiveTexture();
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}

		// 5. Occlusion
		material._occlusionTextureIndex = textureIndexIncrementor++;
		if (gltfMaterial.occlusionTexture.has_value()) 
		{
			size_t textureIndex = gltfMaterial.occlusionTexture->textureIndex;

			const fastgltf::Texture& assetTexture = asset->textures[textureIndex];
			size_t imageIndex = assetTexture.imageIndex.value();
			const fastgltf::Image& assetImage = asset->images[imageIndex];

			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::OCCLUSION;
			ScratchImage scratchImage = ExtractImageFromBuffer(asset.get(), assetImage);
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}
		else
		{
			Texture::TEXTURETYPE texType = Texture::TEXTURETYPE::OCCLUSION;
			ScratchImage scratchImage = LoadFallbackOcclusionTexture();
			textures.emplace_back(Texture(commandList, texType, scratchImage));
		}
		materials.push_back(material);
	}

	std::vector<ModelNode> modelNodes;
	INT nodeIdIncrementor = 0;
	modelNodes.reserve(asset->nodes.size());

	for (const fastgltf::Node& node : asset->nodes) 
	{
		ModelNode modelNode;
		modelNode._id = nodeIdIncrementor++;
		modelNode._name = node.name.data() ? node.name : "UnnamedNode";
		modelNode._meshIndex = node.meshIndex.has_value() ? static_cast<int>(*node.meshIndex) : -1;

		modelNode._localMatrix = ToXMFloat4x4(fastgltf::getTransformMatrix(node));

		XMMATRIX M = XMLoadFloat4x4(&modelNode._localMatrix);
		XMVECTOR translation, rotation, scale;
		XMMatrixDecompose(&scale, &rotation, &translation, M);

		XMStoreFloat3(&modelNode._translation, translation);
		XMStoreFloat3(&modelNode._scale, scale);
		XMStoreFloat4(&modelNode._rotationQuat, rotation);

		modelNodes.push_back(modelNode);
	}

	for (size_t i = 0; i < asset->nodes.size(); ++i) 
	{
		const fastgltf::Node& node = asset->nodes[i];
		for (uint32_t childIndex : node.children) {
			modelNodes[i]._children.push_back(static_cast<int>(childIndex));
			modelNodes[childIndex]._parentIndex = static_cast<int>(i);
		}
	}
	
	model = std::make_shared<Model>(_modelIdIncrementor++, modelName, commandList, meshes, std::move(textures), materials, modelNodes);
}

ScratchImage GLTFLoader::ExtractImageFromBuffer(const fastgltf::Asset& asset, const fastgltf::Image& assetImage)
{
	const uint8_t* pixelData = nullptr;
	size_t pixelSize = 0;
	ScratchImage scratchImage;

	if (auto bufferViewPtr = std::get_if<fastgltf::sources::BufferView>(&assetImage.data)) 
	{
		const fastgltf::sources::BufferView& view = *bufferViewPtr;
		const auto& bufferViewMeta = asset.bufferViews[view.bufferViewIndex];
		const auto& textureName = bufferViewMeta.name;
		const auto& buffer = asset.buffers[bufferViewMeta.bufferIndex];

		if (auto arrayPtr = std::get_if<fastgltf::sources::Array>(&buffer.data)) 
		{
			pixelData = reinterpret_cast<const uint8_t*>(arrayPtr->bytes.data()) + bufferViewMeta.byteOffset;
			pixelSize = bufferViewMeta.byteLength;
		}
	}

	if (!pixelData || pixelSize == 0)
		ThrowException("no pixeldata while loading image");

	ThrowIfFailed(LoadFromWICMemory(pixelData, pixelSize, WIC_FLAGS_NONE, nullptr, scratchImage));

	return scratchImage;
}

void GLTFLoader::ExtractIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<uint32_t>& indices)
{
	const fastgltf::Accessor& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];

	indices.reserve(indexAccessor.count);

	fastgltf::iterateAccessor<std::uint32_t>(asset, indexAccessor, [&](std::uint32_t idx) {
		indices.push_back(idx);
		});
}

void GLTFLoader::ExtractVertices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<Vertex>& vertices, bool& generateTangents)
{
	const fastgltf::Accessor& positionAccessor = asset.accessors[primitive.findAttribute("POSITION")->accessorIndex];
	vertices.resize(vertices.size() + positionAccessor.count);

	fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, positionAccessor, [&](fastgltf::math::fvec3 pos, std::size_t idx) {
		vertices[idx].position = XMFLOAT3(pos.x(), pos.y(), pos.z());
		});

	if (primitive.findAttribute("NORMAL") != primitive.attributes.end())
	{
		const fastgltf::Accessor& normalAccessor = asset.accessors[primitive.findAttribute("NORMAL")->accessorIndex];
		fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, normalAccessor, [&](fastgltf::math::fvec3 normal, std::size_t idx) {
			vertices[idx].normal = XMFLOAT3(normal.x(), normal.y(), normal.z());
			});
	}

	if (primitive.findAttribute("TEXCOORD_0") != primitive.attributes.end())
	{
		const fastgltf::Accessor& uvAccessor = asset.accessors[primitive.findAttribute("TEXCOORD_0")->accessorIndex];

		fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, uvAccessor, [&](fastgltf::math::fvec2 uv, std::size_t idx) {
			vertices[idx].uv = XMFLOAT2(uv.x(), uv.y());
			});
	}

	if (primitive.findAttribute("TANGENT") != primitive.attributes.end())
	{
		const fastgltf::Accessor& tangentAccessor = asset.accessors[primitive.findAttribute("TANGENT")->accessorIndex];

		fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, tangentAccessor, [&](fastgltf::math::fvec4 tangent, std::size_t idx) {
			vertices[idx].tangent = XMFLOAT4(tangent.x(), tangent.y(), tangent.z(), tangent.w());
			});
	}
	else
	{
		generateTangents = true;
	}
}

ScratchImage GLTFLoader::Create1x1Texture(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	ScratchImage image;
	image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
	uint8_t* pixels = image.GetImage(0, 0, 0)->pixels;
	pixels[0] = r;
	pixels[1] = g;
	pixels[2] = b;
	pixels[3] = a;
	return image;
}

ScratchImage GLTFLoader::LoadFallbackAlbedoTexture()
{
	// Default albedo: mid-gray (e.g., base color = 0.5)
	return Create1x1Texture(255, 0, 200);
}

ScratchImage GLTFLoader::LoadFallbackMetallicRoughnessTexture()
{
	// Default: fully rough (255), non-metal (0)
	// In glTF, R = Occlusion, G = Roughness, B = Metallic
	return Create1x1Texture(0, 255, 0);
}

ScratchImage GLTFLoader::LoadFallbackNormalTexture()
{
	// Default normal pointing along +Z in tangent space
	return Create1x1Texture(128, 128, 255);
}

ScratchImage GLTFLoader::LoadFallbackEmissiveTexture()
{
	// Default: black (no emission)
	return Create1x1Texture(0, 0, 0);
}

ScratchImage GLTFLoader::LoadFallbackOcclusionTexture()
{
	// Default: no occlusion
	return Create1x1Texture(255, 255, 255);
}

void GLTFLoader::GenerateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
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