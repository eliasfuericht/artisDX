#include "Model.h"

Model::Model(INT id, std::string name, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<Mesh> meshes, std::vector<Texture> textures, std::vector<Material> materials, std::vector<ModelNode> modelNodes)
{
	_id = id;
	_name = name;
	_meshes = meshes;
	_textures = std::move(textures);
	_materials = materials;
	_modelNodes = modelNodes;
}

void Model::DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ComputeGlobalTransforms();

	for (ModelNode& node : _modelNodes)
	{
		if (node._meshIndex == -1)
			continue;

		Mesh& mesh = _meshes[node._meshIndex];

		memcpy(node._mappedCBVModelMatrixPtr, &node._globalMatrix, sizeof(XMFLOAT4X4));
		commandList->SetGraphicsRootDescriptorTable(1, node._cbvModelMatrixGpuHandle);

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

void Model::ComputeGlobalTransforms() {
	for (int i = 0; i < _modelNodes.size(); ++i) {
		if (_modelNodes[i]._parentIndex == -1) {
			ComputeNodeGlobal(i, XMMatrixIdentity());
		}
	}
}

void Model::ComputeNodeGlobal(int nodeIndex, const XMMATRIX& parentMatrix) {
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

	for (int childIndex : modelNode._children) {
		ComputeNodeGlobal(childIndex, global);
	}
}

INT Model::GetID()
{
	return _id;
}

void Model::DrawGUI() {
	std::string windowName = "Model: " + _name + " ID: " + std::to_string(_id);
	GUI::Begin(windowName.c_str());
	GUI::PushID(_id);
	GUI::PopID();

	for (ModelNode& node : _modelNodes)
	{
		GUI::PushID(node._id);
		GUI::DragFloat3("Translation", node._translation);
		GUI::DragFloat3("Rotation", node._rotationEuler);
		GUI::DragFloat3("Scale", node._scale);
		GUI::PopID();
	}

	GUI::End();
}