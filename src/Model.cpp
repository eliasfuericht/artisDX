#include "Model.h"
Model::Model(	INT id, MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<std::vector<Vertex>> meshInstanceVertices,
							std::vector<std::vector<uint32_t>> meshInstanceIndices, std::vector<XMFLOAT4X4> meshInstanceMatrices, std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures)
{
	_ID = id;

	for (int i = 0; i < meshInstanceVertices.size(); i++)
	{
		_meshInstances.emplace_back(MeshInstance(_meshInstanceId++, Mesh(device, meshInstanceVertices[i], meshInstanceIndices[i]), AABB(device, meshInstanceVertices[i]), meshInstanceMatrices[i], device));
	}

	for (int i = 0; i < textures.size(); i++)
	{
		auto& [texType, texImage] = textures[i];
		Texture modelTexture = Texture(device, commandList, texType, texImage);
		_textures.push_back(std::move(modelTexture));
	}

	ExtractTransformsFromMatrix();
	UpdateModelMatrix();
}

void Model::RegisterWithGUI()
{
	GUI::RegisterComponent(weak_from_this());
}

void Model::DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	for (Texture& texture : _textures)
	{
		texture.BindTexture(commandList);
	}

	for (MeshInstance& meshInstance : _meshInstances)
	{
		memcpy(meshInstance.mappedPtr, &meshInstance.localTransform, sizeof(XMFLOAT4X4));

		commandList->SetGraphicsRootDescriptorTable(1, meshInstance.cbvGpuHandle);
		meshInstance.mesh.BindMeshData(commandList);
	}
}

void Model::ExtractTransformsFromMatrix()
{
	for (MeshInstance& meshInstance : _meshInstances)
	{
		XMMATRIX M = XMLoadFloat4x4(&meshInstance.localTransform);

		XMVECTOR translation, rotation, scale;
		XMMatrixDecompose(&scale, &rotation, &translation, M);

		XMStoreFloat3(&meshInstance.translation, translation);

		XMStoreFloat3(&meshInstance.scaling, scale);

		XMFLOAT4 quat;
		XMStoreFloat4(&quat, rotation);

		XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

		meshInstance.rotation.x = XMConvertToDegrees(atan2(rotationMatrix.r[1].m128_f32[2], rotationMatrix.r[2].m128_f32[2])); // Pitch
		meshInstance.rotation.y = XMConvertToDegrees(atan2(-rotationMatrix.r[0].m128_f32[2], sqrt(rotationMatrix.r[0].m128_f32[0] * rotationMatrix.r[0].m128_f32[0] +
			rotationMatrix.r[0].m128_f32[1] * rotationMatrix.r[0].m128_f32[1]))); // Yaw
		meshInstance.rotation.z = XMConvertToDegrees(atan2(rotationMatrix.r[0].m128_f32[1], rotationMatrix.r[0].m128_f32[0])); // Roll
	}
}

INT Model::GetID()
{
	return _ID;
}

void Model::DrawGUI() {
	std::string windowName = "Model Window " + std::to_string(_ID);

	GUI::Begin(windowName.c_str());
	GUI::PushID(_ID);

	for (MeshInstance& meshInstance : _meshInstances)
	{
		GUI::PushID(meshInstance.id);
		std::string subWindowString = "MeshInstance " + std::to_string(meshInstance.id);
		GUI::Text(subWindowString.c_str());
		GUI::DragFloat3("Translation", meshInstance.translation);
		GUI::DragFloat3("Rotation", meshInstance.rotation);
		GUI::DragFloat3("Scaling", meshInstance.scaling);
		GUI::PopID();
	}

	//_markedForDeletion = GUI::Button("Delete");

	GUI::PopID();
	GUI::End();

	UpdateModelMatrix();
}

void Model::UpdateModelMatrix() 
{
	for (MeshInstance& meshInstance : _meshInstances)
	{
		XMMATRIX modelMatrix = XMMatrixIdentity();

		modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixScaling(meshInstance.scaling.x, meshInstance.scaling.y, meshInstance.scaling.z));

		modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(meshInstance.rotation.x),
			XMConvertToRadians(meshInstance.rotation.y),
			XMConvertToRadians(meshInstance.rotation.z)
		));

		modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixTranslation(meshInstance.translation.x, meshInstance.translation.y, meshInstance.translation.z));

		XMStoreFloat4x4(&meshInstance.localTransform, modelMatrix);
	}
}

void Model::Translate(XMFLOAT3 vec)
{
	for (MeshInstance& meshInstance : _meshInstances)
	{
		XMMATRIX translationMatrix = XMMatrixTranslation(vec.x, vec.y, vec.z);
		XMMATRIX modelMatrix = XMLoadFloat4x4(&meshInstance.localTransform);
		modelMatrix = XMMatrixMultiply(translationMatrix, modelMatrix); // Translation first

		XMStoreFloat4x4(&meshInstance.localTransform, modelMatrix);
	}
}

void Model::Rotate(XMFLOAT3 vec)
{
	for (MeshInstance& meshInstance : _meshInstances)
	{
		XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(vec.x, vec.y, vec.z);
		XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quaternion);
		XMMATRIX modelMatrix = XMLoadFloat4x4(&meshInstance.localTransform);
		modelMatrix = XMMatrixMultiply(rotationMatrix, modelMatrix); // Rotation second

		XMStoreFloat4x4(&meshInstance.localTransform, modelMatrix);
	}
}

void Model::Scale(XMFLOAT3 vec)
{
	for (MeshInstance& meshInstance : _meshInstances)
	{
		XMMATRIX scalingMatrix = XMMatrixScaling(vec.x, vec.y, vec.z);
		XMMATRIX modelMatrix = XMLoadFloat4x4(&meshInstance.localTransform);
		modelMatrix = XMMatrixMultiply(scalingMatrix, modelMatrix); // Scale last

		XMStoreFloat4x4(&meshInstance.localTransform, modelMatrix);
	}
}