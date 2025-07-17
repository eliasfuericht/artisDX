#include "Model.h"

Model::Model(int32_t id, std::string name, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<Mesh> meshes, std::vector<Texture> textures, std::vector<Material> materials, std::vector<ModelNode> modelNodes)
{
	_id = id;
	_name = name;
	_meshes = meshes;
	_textures = std::move(textures);
	_materials = materials;
	_modelNodes = modelNodes;
	XMStoreFloat4x4(&_globalMatrix, XMMatrixIdentity()) ;
}

void Model::DrawModel(const ShaderPass& shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ComputeGlobalTransforms();

	for (ModelNode& node : _modelNodes)
	{
		if (node._meshIndex == -1)
			continue;

		Mesh& mesh = _meshes[node._meshIndex];

		node.BindModelMatrixData(shaderPass, commandList);

		std::vector<Primitive> transparentPrimitives;

		for (Primitive& primitive : mesh._primitives)
		{
			Material& material = _materials[primitive._materialIndex];
			if (material._alphaMode == fastgltf::AlphaMode::Blend || material._alphaMode == fastgltf::AlphaMode::Mask)
			{
				transparentPrimitives.push_back(primitive);
				continue;
			}

			material._baseColorTextureIndex != NOTOK ? _textures[material._baseColorTextureIndex].BindTexture(shaderPass, commandList) : PRINT("baseColorTextureIndex NOTOK");
			material._metallicRoughnessTextureIndex != NOTOK ? _textures[material._metallicRoughnessTextureIndex].BindTexture(shaderPass, commandList) : PRINT("metallicRoughnessTextureIndex NOTOK");
			material._normalTextureIndex != NOTOK ? _textures[material._normalTextureIndex].BindTexture(shaderPass, commandList) : PRINT("normalTextureIndex NOTOK");
			material._emissiveTextureIndex != NOTOK ? _textures[material._emissiveTextureIndex].BindTexture(shaderPass, commandList) : PRINT("emissiveTextureIndex NOTOK");
			material._occlusionTextureIndex != NOTOK ? _textures[material._occlusionTextureIndex].BindTexture(shaderPass, commandList) : PRINT("occlusionTextureIndex NOTOK");
			material.BindMaterialFactorsData(shaderPass, commandList);
			primitive.BindPrimitiveData(commandList);
		}

		for (Primitive& primitive : transparentPrimitives)
		{
			Material& material = _materials[primitive._materialIndex];

			material._baseColorTextureIndex != NOTOK ? _textures[material._baseColorTextureIndex].BindTexture(shaderPass, commandList) : PRINT("baseColorTextureIndex NOTOK");
			material._metallicRoughnessTextureIndex != NOTOK ? _textures[material._metallicRoughnessTextureIndex].BindTexture(shaderPass, commandList) : PRINT("metallicRoughnessTextureIndex NOTOK");
			material._normalTextureIndex != NOTOK ? _textures[material._normalTextureIndex].BindTexture(shaderPass, commandList) : PRINT("normalTextureIndex NOTOK");
			material._emissiveTextureIndex != NOTOK ? _textures[material._emissiveTextureIndex].BindTexture(shaderPass, commandList) : PRINT("emissiveTextureIndex NOTOK");
			material._occlusionTextureIndex != NOTOK ? _textures[material._occlusionTextureIndex].BindTexture(shaderPass, commandList) : PRINT("occlusionTextureIndex NOTOK");
			material.BindMaterialFactorsData(shaderPass, commandList);
			primitive.BindPrimitiveData(commandList);
		}
	}
}

void Model::DrawModelBoundingBox(const ShaderPass& shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	for (ModelNode& node : _modelNodes)
	{
		if (node._meshIndex == -1)
			continue;

		Mesh& mesh = _meshes[node._meshIndex];

		node.BindModelMatrixData(shaderPass, commandList);

		for (Primitive& primitive : mesh._primitives)
		{
			primitive._aabb.BindMeshData(commandList);
		}
	}
}

void Model::ComputeGlobalTransforms() 
{
	XMMATRIX local = XMMatrixScalingFromVector(XMLoadFloat3(&_scale)) * XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&_rotationEuler)) * XMMatrixTranslationFromVector(XMLoadFloat3(&_translation));

	XMStoreFloat4x4(&_globalMatrix, XMMatrixMultiply(XMLoadFloat4x4(&_globalMatrix), local));

	for (size_t i = 0; i < _modelNodes.size(); ++i) 
	{
		if (_modelNodes[i]._parentIndex == -1) 
		{
			ComputeNodeGlobal(i, XMLoadFloat4x4(&_globalMatrix));
		}
	}

	XMStoreFloat4x4(&_globalMatrix, XMMatrixIdentity());
}

void Model::ComputeNodeGlobal(int32_t nodeIndex, const XMMATRIX& parentMatrix)
{
	ModelNode& modelNode = _modelNodes[nodeIndex];

	XMVECTOR originalQuat = XMLoadFloat4(&modelNode._rotationQuat);

	XMVECTOR userQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&modelNode._rotationEuler));

	XMStoreFloat3(&modelNode._rotationEuler, XMVECTOR());

	XMVECTOR combinedQuat = XMQuaternionMultiply(userQuat, originalQuat);

	combinedQuat = XMQuaternionNormalize(combinedQuat);

	XMStoreFloat4(&modelNode._rotationQuat, combinedQuat);

	XMMATRIX local =	XMMatrixScalingFromVector(XMLoadFloat3(&modelNode._scale)) * 
										XMMatrixRotationQuaternion(XMLoadFloat4(&modelNode._rotationQuat)) * 
										XMMatrixTranslationFromVector(XMLoadFloat3(&modelNode._translation));

	XMStoreFloat4x4(&modelNode._localMatrix, local);

	XMMATRIX global = local * parentMatrix;
	XMStoreFloat4x4(&modelNode._globalMatrix, global);

	for (int childIndex : modelNode._children) 
	{
		ComputeNodeGlobal(childIndex, global);
	}
}

int32_t Model::GetID()
{
	return _id;
}

void Model::DrawGUI() {
	std::string windowName = "Model: " + _name + " ID: " + std::to_string(_id);
	GUI::Begin(windowName.c_str());
	GUI::Text("Model Transforms");
	GUI::DragFloat3("Translation", _translation);
	GUI::DragFloat3("Rotation", _rotationEuler);
	GUI::DragFloat3("Scale", _scale);

	for (ModelNode& node : _modelNodes)
	{
		if (node._meshIndex != NOTOK)
		{
			GUI::PushID(node._id);
			std::string nodeString = "MeshNode: " + node._name + " ID: " + std::to_string(node._id);
			GUI::Text(nodeString.c_str());
			GUI::DragFloat3("Translation", node._translation);
			GUI::DragFloat3("Rotation", node._rotationEuler);
			GUI::DragFloat3("Scale", node._scale);
			GUI::PopID();
		}
	}

	GUI::End();
}