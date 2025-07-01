#include "Model.h"
Model::Model(	INT id, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<std::vector<Vertex>> meshInstanceVertices, std::vector<std::vector<uint32_t>> meshInstanceIndices,
							std::vector<XMFLOAT4X4> meshInstanceMatrices, std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures, std::vector<std::tuple<INT, std::vector<INT>>> materials)
{
	_ID = id;

	for (int i = 0; i < meshInstanceVertices.size(); i++)
	{
		auto& [materialIndex, materialTextureIndices] = materials[i];

		_meshInstances.emplace_back(MeshInstance(_meshInstanceId++, Mesh(meshInstanceVertices[i], meshInstanceIndices[i]), AABB(meshInstanceVertices[i]), meshInstanceMatrices[i], materialIndex));
		_materialTextureIndices.push_back(materialTextureIndices);
	}

	for (int i = 0; i < textures.size(); i++)
	{
		auto& [texType, texImage] = textures[i];
		Texture modelTexture = Texture(commandList, texType, texImage);
		_textures.push_back(std::move(modelTexture));
	}

	XMStoreFloat4x4(&_transformMatrix, XMLoadFloat4x4(&meshInstanceMatrices[0]));

	ExtractTransformsFromMatrix();
	UpdateTransformMatrix();
}

void Model::DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{

	for (MeshInstance& meshInstance : _meshInstances)
	{
		memcpy(meshInstance._mappedPtr, &meshInstance._localTransform, sizeof(XMFLOAT4X4));

		commandList->SetGraphicsRootDescriptorTable(1, meshInstance._cbvGpuHandle);

		for (INT i : _materialTextureIndices[meshInstance._materialIndex])
		{
			_textures[i].BindTexture(commandList);
		}

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
	std::string partenTransformText = "Parent Transform";
	GUI::Text(partenTransformText.c_str());
	GUI::DragFloat3("Parent Translation", _translation);
	GUI::DragFloat3("Parent Rotation", _rotation);
	GUI::DragFloat3("Parent Scaling", _scaling);

	GUI::PopID();

	for (MeshInstance& meshInstance : _meshInstances)
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
	for (MeshInstance& meshInstance : _meshInstances)
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
	for (MeshInstance& meshInstance : _meshInstances)
	{
		XMMATRIX translationMatrix = XMMatrixTranslation(vec.x, vec.y, vec.z);
		XMMATRIX transformMatrix = XMLoadFloat4x4(&meshInstance._localTransform);
		transformMatrix = XMMatrixMultiply(translationMatrix, transformMatrix); // Translation first

		XMStoreFloat4x4(&meshInstance._localTransform, transformMatrix);
	}
}

void Model::Rotate(XMFLOAT3 vec)
{
	for (MeshInstance& meshInstance : _meshInstances)
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
	for (MeshInstance& meshInstance : _meshInstances)
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