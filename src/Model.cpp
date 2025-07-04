#include "Model.h"

Model::Model(INT id, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<Mesh> meshes, std::vector<Texture> textures, std::vector<Material> materials)
{
	_id = id;
	_meshes = meshes;
	_textures = std::move(textures);
	_materials = materials;
	//_modelNodes = modelNodes;
}

void Model::DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	for (Mesh& mesh : _meshes)
	{
		memcpy(mesh._mappedPtr, &mesh._localTransform, sizeof(XMFLOAT4X4));

		commandList->SetGraphicsRootDescriptorTable(1, mesh._cbvGpuHandle);

		for (Primitive& primitive : mesh._primitives)
		{
			Material& material = _materials[primitive._materialIndex];

			material._baseColorTextureIndex != NOTOK ? _textures[material._baseColorTextureIndex].BindTexture(commandList) : PRINT("baseColorTextureIndex NOTOK");
			material._metallicRoughnessTextureIndex != NOTOK ? _textures[material._metallicRoughnessTextureIndex].BindTexture(commandList) : PRINT("metallicRoughnessTextureIndex NOTOK");
			material._normalTextureIndex != NOTOK ? _textures[material._normalTextureIndex].BindTexture(commandList) : PRINT("normalTextureIndex NOTOK");
			material._emissiveTextureIndex != NOTOK ? _textures[material._emissiveTextureIndex].BindTexture(commandList) : PRINT("emissiveTextureIndex NOTOK");
			material._occlusionTextureIndex != NOTOK ? _textures[material._occlusionTextureIndex].BindTexture(commandList) : PRINT("occlusionTextureIndex NOTOK");
			primitive.BindPrimitiveData(commandList);
		}
	}
}

INT Model::GetID()
{
	return _id;
}

void Model::DrawGUI() {
	std::string windowName = "Model Window " + std::to_string(_id);

	GUI::Begin(windowName.c_str());
	GUI::PushID(_id);
	std::string partenTransformText = "Parent Transform";
	GUI::Text(partenTransformText.c_str());
	//GUI::DragFloat3("Parent Translation", _translation);
	//GUI::DragFloat3("Parent Rotation", _rotation);
	//GUI::DragFloat3("Parent Scaling", _scaling);

	GUI::PopID();

	for (Mesh& mesh : _meshes)
	{
		GUI::PushID(mesh._id);
		std::string meshTransformText = "Mesh " + std::to_string(mesh._id);
		GUI::Text(meshTransformText.c_str());
		GUI::DragFloat3("Translation", mesh._translation);
		GUI::DragFloat3("Rotation", mesh._rotation);
		GUI::DragFloat3("Scaling", mesh._scaling);
		GUI::PopID();
	}

	GUI::End();
}