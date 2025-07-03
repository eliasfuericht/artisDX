#include "Model.h"

Model::Model(INT id, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<Mesh> meshes, std::vector<Texture> textures, std::vector<Material> materials)
{
	_id = id;
	_meshes = meshes;
	_textures = std::move(textures);
	_materials = materials;

	ExtractTransformsFromMatrix();
	UpdateTransformMatrix();
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

			material.baseColorTextureIndex != NOTOK ? _textures[material.baseColorTextureIndex].BindTexture(commandList) : PRINT("baseColorTextureIndex NOTOK");
			material.metallicRoughnessTextureIndex != NOTOK ? _textures[material.metallicRoughnessTextureIndex].BindTexture(commandList) : PRINT("metallicRoughnessTextureIndex NOTOK");
			material.normalTextureIndex != NOTOK ? _textures[material.normalTextureIndex].BindTexture(commandList) : PRINT("normalTextureIndex NOTOK");
			material.emissiveTextureIndex != NOTOK ? _textures[material.emissiveTextureIndex].BindTexture(commandList) : PRINT("emissiveTextureIndex NOTOK");
			material.occlusionTextureIndex != NOTOK ? _textures[material.occlusionTextureIndex].BindTexture(commandList) : PRINT("occlusionTextureIndex NOTOK");
			primitive.BindPrimitiveData(commandList);
		}
	}
}

void Model::ExtractTransformsFromMatrix()
{
	for (Mesh& meshInstance : _meshes)
	{
		XMMATRIX M = XMLoadFloat4x4(&meshInstance._localTransform);

		XMVECTOR translation, rotation, scale;
		XMMatrixDecompose(&scale, &rotation, &translation, M);

		XMStoreFloat3(&meshInstance._translation, translation);

		XMStoreFloat3(&meshInstance._scaling, scale);

		XMFLOAT4 quat;
		XMStoreFloat4(&quat, rotation);

		XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

		meshInstance._rotation.x = XMConvertToDegrees(atan2(rotationMatrix.r[1].m128_f32[2], rotationMatrix.r[2].m128_f32[2])); // Pitch
		meshInstance._rotation.y = XMConvertToDegrees(atan2(-rotationMatrix.r[0].m128_f32[2], sqrt(rotationMatrix.r[0].m128_f32[0] * rotationMatrix.r[0].m128_f32[0] +
			rotationMatrix.r[0].m128_f32[1] * rotationMatrix.r[0].m128_f32[1]))); // Yaw
		meshInstance._rotation.z = XMConvertToDegrees(atan2(rotationMatrix.r[0].m128_f32[1], rotationMatrix.r[0].m128_f32[0])); // Roll
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
	GUI::DragFloat3("Parent Translation", _translation);
	GUI::DragFloat3("Parent Rotation", _rotation);
	GUI::DragFloat3("Parent Scaling", _scaling);

	GUI::PopID();

	for (Mesh& meshInstance : _meshes)
	{
		GUI::PushID(meshInstance._id);
		std::string meshInstanceTransformText = "MeshInstance " + std::to_string(meshInstance._id);
		GUI::Text(meshInstanceTransformText.c_str());
		GUI::DragFloat3("Translation", meshInstance._translation);
		GUI::DragFloat3("Rotation", meshInstance._rotation);
		GUI::DragFloat3("Scaling", meshInstance._scaling);
		GUI::PopID();
	}

	GUI::End();

	UpdateTransformMatrix();
}

void Model::UpdateTransformMatrix()
{
	// update parent transformMatrix
	XMMATRIX parentTransform = XMMatrixIdentity();

	parentTransform = XMMatrixMultiply(parentTransform, XMMatrixScaling(_scaling.x, _scaling.y, _scaling.z));

	parentTransform = XMMatrixMultiply(parentTransform, XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(_rotation.x),
		XMConvertToRadians(_rotation.y),
		XMConvertToRadians(_rotation.z)
	));

	parentTransform = XMMatrixMultiply(parentTransform, XMMatrixTranslation(_translation.x, _translation.y, _translation.z));

	XMStoreFloat4x4(&_transformMatrix, parentTransform);

	// update MeshInstance transformMatrix
	for (Mesh& meshInstance : _meshes)
	{
		XMMATRIX localTransform = XMMatrixIdentity();

		localTransform = XMMatrixMultiply(localTransform, XMMatrixScaling(meshInstance._scaling.x, meshInstance._scaling.y, meshInstance._scaling.z));

		localTransform = XMMatrixMultiply(localTransform, XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(meshInstance._rotation.x),
			XMConvertToRadians(meshInstance._rotation.y),
			XMConvertToRadians(meshInstance._rotation.z)
		));

		localTransform = XMMatrixMultiply(localTransform, XMMatrixTranslation(meshInstance._translation.x, meshInstance._translation.y, meshInstance._translation.z));

		XMMATRIX worldTransform = XMMatrixMultiply(localTransform, parentTransform);

		XMStoreFloat4x4(&meshInstance._localTransform, worldTransform);
	}
}

void Model::Translate(XMFLOAT3 vec)
{
	for (Mesh& meshInstance : _meshes)
	{
		XMMATRIX translationMatrix = XMMatrixTranslation(vec.x, vec.y, vec.z);
		XMMATRIX transformMatrix = XMLoadFloat4x4(&meshInstance._localTransform);
		transformMatrix = XMMatrixMultiply(translationMatrix, transformMatrix); // Translation first

		XMStoreFloat4x4(&meshInstance._localTransform, transformMatrix);
	}
}

void Model::Rotate(XMFLOAT3 vec)
{
	for (Mesh& meshInstance : _meshes)
	{
		XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(vec.x, vec.y, vec.z);
		XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quaternion);
		XMMATRIX transformMatrix = XMLoadFloat4x4(&meshInstance._localTransform);
		transformMatrix = XMMatrixMultiply(rotationMatrix, transformMatrix); // Rotation second

		XMStoreFloat4x4(&meshInstance._localTransform, transformMatrix);
	}
}

void Model::Scale(XMFLOAT3 vec)
{
	for (Mesh& meshInstance : _meshes)
	{
		XMMATRIX scalingMatrix = XMMatrixScaling(vec.x, vec.y, vec.z);
		XMMATRIX transformMatrix = XMLoadFloat4x4(&meshInstance._localTransform);
		transformMatrix = XMMatrixMultiply(scalingMatrix, transformMatrix); // Scale last

		XMStoreFloat4x4(&meshInstance._localTransform, transformMatrix);
	}
}

void Model::RegisterWithGUI()
{
	GUI::RegisterComponent(weak_from_this());
}