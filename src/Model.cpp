#include "Model.h"
Model::Model(	INT id, MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<std::vector<Vertex>> submeshVertices, 
							std::vector<std::vector<uint32_t>> submeshIndices, std::vector<XMFLOAT4X4> submeshMatrices, std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures)
{
	_ID = id;

	for (int i = 0; i < submeshVertices.size(); i++)
	{
		_subMeshes.emplace_back(SubMesh(_subMeshId++, Mesh(device, submeshVertices[i], submeshIndices[i]), AABB(device, submeshVertices[i]), submeshMatrices[i], device));
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

	for (SubMesh& subMesh : _subMeshes)
	{
		memcpy(subMesh.mappedPtr, &subMesh.localTransform, sizeof(XMFLOAT4X4));

		commandList->SetGraphicsRootDescriptorTable(1, subMesh.cbvGpuHandle);
		subMesh.mesh.BindMeshData(commandList);
	}
}

void Model::ExtractTransformsFromMatrix()
{
	for (SubMesh& subMesh : _subMeshes)
	{
		XMMATRIX M = XMLoadFloat4x4(&subMesh.localTransform);

		XMVECTOR translation, rotation, scale;
		XMMatrixDecompose(&scale, &rotation, &translation, M);

		XMStoreFloat3(&subMesh.translation, translation);

		XMStoreFloat3(&subMesh.scaling, scale);

		XMFLOAT4 quat;
		XMStoreFloat4(&quat, rotation);

		XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

		subMesh.rotation.x = XMConvertToDegrees(atan2(rotationMatrix.r[1].m128_f32[2], rotationMatrix.r[2].m128_f32[2])); // Pitch
		subMesh.rotation.y = XMConvertToDegrees(atan2(-rotationMatrix.r[0].m128_f32[2], sqrt(rotationMatrix.r[0].m128_f32[0] * rotationMatrix.r[0].m128_f32[0] +
			rotationMatrix.r[0].m128_f32[1] * rotationMatrix.r[0].m128_f32[1]))); // Yaw
		subMesh.rotation.z = XMConvertToDegrees(atan2(rotationMatrix.r[0].m128_f32[1], rotationMatrix.r[0].m128_f32[0])); // Roll
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

	for (SubMesh& subMesh : _subMeshes)
	{
		GUI::PushID(subMesh.id);
		std::string subWindowString = "SubMesh " + std::to_string(subMesh.id);
		GUI::Text(subWindowString.c_str());
		GUI::DragFloat3("Translation", subMesh.translation);
		GUI::DragFloat3("Rotation", subMesh.rotation);
		GUI::DragFloat3("Scaling", subMesh.scaling);
		GUI::PopID();
	}

	//_markedForDeletion = GUI::Button("Delete");

	GUI::PopID();
	GUI::End();

	UpdateModelMatrix();
}

void Model::UpdateModelMatrix() 
{
	for (SubMesh& subMesh : _subMeshes)
	{
		XMMATRIX modelMatrix = XMMatrixIdentity();

		modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixScaling(subMesh.scaling.x, subMesh.scaling.y, subMesh.scaling.z));

		modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(subMesh.rotation.x),
			XMConvertToRadians(subMesh.rotation.y),
			XMConvertToRadians(subMesh.rotation.z)
		));

		modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixTranslation(subMesh.translation.x, subMesh.translation.y, subMesh.translation.z));

		XMStoreFloat4x4(&subMesh.localTransform, modelMatrix);
	}
}

void Model::Translate(XMFLOAT3 vec)
{
	for (SubMesh& subMesh : _subMeshes)
	{
		XMMATRIX translationMatrix = XMMatrixTranslation(vec.x, vec.y, vec.z);
		XMMATRIX modelMatrix = XMLoadFloat4x4(&subMesh.localTransform);
		modelMatrix = XMMatrixMultiply(translationMatrix, modelMatrix); // Translation first

		XMStoreFloat4x4(&subMesh.localTransform, modelMatrix);
	}
}

void Model::Rotate(XMFLOAT3 vec)
{
	for (SubMesh& subMesh : _subMeshes)
	{
		XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(vec.x, vec.y, vec.z);
		XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quaternion);
		XMMATRIX modelMatrix = XMLoadFloat4x4(&subMesh.localTransform);
		modelMatrix = XMMatrixMultiply(rotationMatrix, modelMatrix); // Rotation second

		XMStoreFloat4x4(&subMesh.localTransform, modelMatrix);
	}
}

void Model::Scale(XMFLOAT3 vec)
{
	for (SubMesh& subMesh : _subMeshes)
	{
		XMMATRIX scalingMatrix = XMMatrixScaling(vec.x, vec.y, vec.z);
		XMMATRIX modelMatrix = XMLoadFloat4x4(&subMesh.localTransform);
		modelMatrix = XMMatrixMultiply(scalingMatrix, modelMatrix); // Scale last

		XMStoreFloat4x4(&subMesh.localTransform, modelMatrix);
	}
}