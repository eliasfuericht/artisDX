#include "GLTFLoader.h"

fastgltf::Parser GLTFLoader::_parser;
INT GLTFLoader::_modelIdIncrementor = 0;

void GLTFLoader::ConstructModelFromFile(std::filesystem::path path, Model& model, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
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

	std::vector<Mesh> meshes;
	for (const fastgltf::Mesh& mesh : asset->meshes)
	{
		INT meshIdIncrementor = 0;
		std::vector<Primitive> primitives;

		for (const fastgltf::Primitive& primitive : mesh.primitives)
		{
			std::vector<uint32_t> indices;
			ExtractIndices(asset.get(), primitive, indices);
			std::vector<Vertex> vertices;
			ExtractVertices(asset.get(), primitive, vertices);
			
			primitives.emplace_back(Primitive(vertices, indices));
		}

		meshes.emplace_back(Mesh(meshIdIncrementor++, primitives));
	}

	std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures;
	std::vector<std::tuple<INT, std::vector<INT>>> materials;

	model = Model(_modelIdIncrementor++, commandList, meshes, std::move(textures));
}

std::vector<uint32_t> GLTFLoader::ExtractIndices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<uint32_t>& indices)
{
	const fastgltf::Accessor& indexAccessor = asset.accessors[primitive.indicesAccessor.value()];

	indices.reserve(indexAccessor.count);

	fastgltf::iterateAccessor<std::uint32_t>(asset, indexAccessor, [&](std::uint32_t idx) {
		indices.push_back(idx);
		});

	return indices;
}

std::vector<Vertex> GLTFLoader::ExtractVertices(const fastgltf::Asset& asset, const fastgltf::Primitive& primitive, std::vector<Vertex>& vertices)
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
		//GenerateTangents();
	}

	return vertices;
}