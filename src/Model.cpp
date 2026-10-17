#include "Model.h"

Model::Model(int32_t id, std::string name, std::vector<Mesh> meshes, std::vector<Texture> textures, std::vector<Material> materials, std::vector<ModelNode> modelNodes)
{
	_id = id;
	_name = name;
	_meshes = meshes;
	_textures = std::move(textures);
	_materials = materials;
	_modelNodes = modelNodes;
	XMStoreFloat4x4(&_globalMatrix, XMMatrixIdentity()) ;
}

void Model::AppendDrawItems(std::vector<Model::DrawItem>& drawItems)
{
	const std::weak_ptr<Model> model = std::static_pointer_cast<Model>(shared_from_this());
	for (size_t nodeIndex = 0; nodeIndex < _modelNodes.size(); ++nodeIndex)
	{
		const ModelNode& node = _modelNodes[nodeIndex];
		if (node._meshIndex == NOTOK) continue;
		for (size_t primitiveIndex = 0; primitiveIndex < _meshes[node._meshIndex]._primitives.size(); ++primitiveIndex)
		{
			drawItems.push_back({model, nodeIndex, primitiveIndex});
		}
	}
}

void Model::UpdateDrawItem(Model::DrawItem& item, const XMFLOAT4X4& viewMatrix) const
{
	const ModelNode& node = _modelNodes.at(item.nodeIndex);
	const Primitive& primitive = _meshes.at(node._meshIndex)._primitives.at(item.primitiveIndex);
	item.alphaMode = _materials.at(primitive._materialIndex)._alphaMode;
	if (item.alphaMode == fastgltf::AlphaMode::Blend)
	{
		const XMVECTOR center = (XMLoadFloat3(&primitive._aabb.GetMin()) + XMLoadFloat3(&primitive._aabb.GetMax())) * 0.5f;
		item.viewDepth = XMVectorGetZ(XMVector3TransformCoord(center, XMLoadFloat4x4(&node._globalMatrix) * XMLoadFloat4x4(&viewMatrix)));
	}
}

void Model::DrawPrimitive(const Model::DrawItem& item, std::shared_ptr<ShaderPass> shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ModelNode& node = _modelNodes.at(item.nodeIndex);
	Primitive& primitive = _meshes.at(node._meshIndex)._primitives.at(item.primitiveIndex);
	node.BindModelMatrixData(shaderPass, commandList);
	Material& material = _materials.at(primitive._materialIndex);
	material._baseColorTextureIndex != NOTOK ? _textures[material._baseColorTextureIndex].BindTexture(shaderPass, commandList) : PRINT("baseColorTextureIndex NOTOK");
	material._metallicRoughnessTextureIndex != NOTOK ? _textures[material._metallicRoughnessTextureIndex].BindTexture(shaderPass, commandList) : PRINT("metallicRoughnessTextureIndex NOTOK");
	material._normalTextureIndex != NOTOK ? _textures[material._normalTextureIndex].BindTexture(shaderPass, commandList) : PRINT("normalTextureIndex NOTOK");
	material._emissiveTextureIndex != NOTOK ? _textures[material._emissiveTextureIndex].BindTexture(shaderPass, commandList) : PRINT("emissiveTextureIndex NOTOK");
	material._occlusionTextureIndex != NOTOK ? _textures[material._occlusionTextureIndex].BindTexture(shaderPass, commandList) : PRINT("occlusionTextureIndex NOTOK");
	material.BindMaterialFactorsData(shaderPass, commandList);
	primitive.BindPrimitiveData(commandList);
}

void Model::DrawModelBoundingBox(std::shared_ptr<ShaderPass> shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
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
			ComputeNodeGlobal(static_cast<int32_t>(i), XMLoadFloat4x4(&_globalMatrix));
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
	ImGui::Begin(windowName.c_str());
	ImGui::Text("Model Transforms");
	ImGui::DragFloat3("Translation", &_translation.x, 0.01f);
	ImGui::DragFloat3("Rotation", &_rotationEuler.x, 0.01f);
	ImGui::DragFloat3("Scale", &_scale.x, 0.01f);

	for (ModelNode& node : _modelNodes)
	{
		if (node._meshIndex != NOTOK)
		{
			ImGui::PushID(node._id);
			std::string nodeString = "MeshNode: " + node._name + " ID: " + std::to_string(node._id);
			ImGui::Text(nodeString.c_str());
			ImGui::DragFloat3("Translation", &node._translation.x, 0.01f);
			ImGui::DragFloat3("Rotation", &node._rotationEuler.x, 0.01f);
			ImGui::DragFloat3("Scale", &node._scale.x, 0.01f);
			ImGui::PopID();
		}
	}

	ImGui::End();
}
