#include "Model.h"
Model::Model(	INT id, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<std::vector<Vertex>> meshInstanceVertices, std::vector<std::vector<uint32_t>> meshInstanceIndices,
							std::vector<XMFLOAT4X4> meshInstanceMatrices, std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures, std::vector<INT> materialIndices, std::vector<std::vector<INT>> materials)
{
	_ID = id;

	for (int i = 0; i < meshInstanceVertices.size(); i++)
	{
		_meshInstances.emplace_back(MeshInstance(_meshInstanceId++, Mesh(meshInstanceVertices[i], meshInstanceIndices[i]), AABB(meshInstanceVertices[i]), meshInstanceMatrices[i], materialIndices[i]));
	}

	for (int i = 0; i < textures.size(); i++)
	{
		auto& [texType, texImage] = textures[i];
		Texture modelTexture = Texture(commandList, texType, texImage);
		_textures.push_back(std::move(modelTexture));
	}

	//for (int i = 0; i < materials.size(); i++) {	}

	ExtractTransformsFromMatrix();
	UpdateModelMatrix();
}

void Model::DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_textures[0].BindTexture(commandList);

	for (MeshInstance& meshInstance : _meshInstances)
	{
		memcpy(meshInstance._mappedPtr, &meshInstance._localTransform, sizeof(XMFLOAT4X4));

		commandList->SetGraphicsRootDescriptorTable(1, meshInstance._cbvGpuHandle);
		/*
		const Material& material = _materials[meshInstance._materialIndex];
		commandList->SetGraphicsRootDescriptorTable(2, material._gpuHandle);
		*/
		meshInstance._mesh.BindMeshData(commandList);
	}
}

void Model::ExtractTransformsFromMatrix()
{
	for (MeshInstance& meshInstance : _meshInstances)
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
	return _ID;
}

void Model::DrawGUI() {
	std::string windowName = "Model Window " + std::to_string(_ID);

	GUI::Begin(windowName.c_str());
	GUI::PushID(_ID);

	for (MeshInstance& meshInstance : _meshInstances)
	{
		GUI::PushID(meshInstance._id);
		std::string subWindowString = "MeshInstance " + std::to_string(meshInstance._id);
		GUI::Text(subWindowString.c_str());
		GUI::DragFloat3("Translation", meshInstance._translation);
		GUI::DragFloat3("Rotation", meshInstance._rotation);
		GUI::DragFloat3("Scaling", meshInstance._scaling);
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

		modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixScaling(meshInstance._scaling.x, meshInstance._scaling.y, meshInstance._scaling.z));

		modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(meshInstance._rotation.x),
			XMConvertToRadians(meshInstance._rotation.y),
			XMConvertToRadians(meshInstance._rotation.z)
		));

		modelMatrix = XMMatrixMultiply(modelMatrix, XMMatrixTranslation(meshInstance._translation.x, meshInstance._translation.y, meshInstance._translation.z));

		XMStoreFloat4x4(&meshInstance._localTransform, modelMatrix);
	}
}

void Model::Translate(XMFLOAT3 vec)
{
	for (MeshInstance& meshInstance : _meshInstances)
	{
		XMMATRIX translationMatrix = XMMatrixTranslation(vec.x, vec.y, vec.z);
		XMMATRIX modelMatrix = XMLoadFloat4x4(&meshInstance._localTransform);
		modelMatrix = XMMatrixMultiply(translationMatrix, modelMatrix); // Translation first

		XMStoreFloat4x4(&meshInstance._localTransform, modelMatrix);
	}
}

void Model::Rotate(XMFLOAT3 vec)
{
	for (MeshInstance& meshInstance : _meshInstances)
	{
		XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(vec.x, vec.y, vec.z);
		XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(quaternion);
		XMMATRIX modelMatrix = XMLoadFloat4x4(&meshInstance._localTransform);
		modelMatrix = XMMatrixMultiply(rotationMatrix, modelMatrix); // Rotation second

		XMStoreFloat4x4(&meshInstance._localTransform, modelMatrix);
	}
}

void Model::Scale(XMFLOAT3 vec)
{
	for (MeshInstance& meshInstance : _meshInstances)
	{
		XMMATRIX scalingMatrix = XMMatrixScaling(vec.x, vec.y, vec.z);
		XMMATRIX modelMatrix = XMLoadFloat4x4(&meshInstance._localTransform);
		modelMatrix = XMMatrixMultiply(scalingMatrix, modelMatrix); // Scale last

		XMStoreFloat4x4(&meshInstance._localTransform, modelMatrix);
	}
}

void Model::RegisterWithGUI()
{
	GUI::RegisterComponent(weak_from_this());
}