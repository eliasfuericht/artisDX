#include "ModelManager.h"

ModelManager::ModelManager(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_commandList = commandList;
}

void ModelManager::LoadModel(std::filesystem::path path)
{
	std::shared_ptr<Model> model;

	_gltfLoader.ConstructModelFromFile(path, model, _commandList);

	model->RegisterWithGUI();

	_models.push_back(std::move(model));
}

void ModelManager::DrawAll()
{
	for (auto& model : _models)
	{
		model->DrawModel(_commandList);
	}
}

void ModelManager::GenerateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
	std::vector<XMFLOAT3> accumulatedTan(vertices.size(), XMFLOAT3(0, 0, 0));
	std::vector<XMFLOAT3> accumulatedBitan(vertices.size(), XMFLOAT3(0, 0, 0));

	for (size_t i = 0; i < indices.size(); i += 3) {
		uint32_t i0 = indices[i + 0];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		const XMFLOAT3& p0 = vertices[i0].position;
		const XMFLOAT3& p1 = vertices[i1].position;
		const XMFLOAT3& p2 = vertices[i2].position;

		const XMFLOAT2& uv0 = vertices[i0].uv;
		const XMFLOAT2& uv1 = vertices[i1].uv;
		const XMFLOAT2& uv2 = vertices[i2].uv;

		XMVECTOR v0 = XMLoadFloat3(&p0);
		XMVECTOR v1 = XMLoadFloat3(&p1);
		XMVECTOR v2 = XMLoadFloat3(&p2);

		XMVECTOR edge1 = XMVectorSubtract(v1, v0);
		XMVECTOR edge2 = XMVectorSubtract(v2, v0);

		float du1 = uv1.x - uv0.x;
		float dv1 = uv1.y - uv0.y;
		float du2 = uv2.x - uv0.x;
		float dv2 = uv2.y - uv0.y;

		float det = du1 * dv2 - du2 * dv1;
		if (fabs(det) < 1e-6f) det = 1.0f;
		float invDet = 1.0f / det;

		XMVECTOR tangent = XMVectorScale(
			XMVectorSubtract(
				XMVectorScale(edge1, dv2),
				XMVectorScale(edge2, dv1)
			), invDet
		);

		XMVECTOR bitangent = XMVectorScale(
			XMVectorSubtract(
				XMVectorScale(edge2, du1),
				XMVectorScale(edge1, du2)
			), invDet
		);

		XMFLOAT3 t, b;
		XMStoreFloat3(&t, tangent);
		XMStoreFloat3(&b, bitangent);

		accumulatedTan[i0].x += t.x; accumulatedTan[i0].y += t.y; accumulatedTan[i0].z += t.z;
		accumulatedTan[i1].x += t.x; accumulatedTan[i1].y += t.y; accumulatedTan[i1].z += t.z;
		accumulatedTan[i2].x += t.x; accumulatedTan[i2].y += t.y; accumulatedTan[i2].z += t.z;

		accumulatedBitan[i0].x += b.x; accumulatedBitan[i0].y += b.y; accumulatedBitan[i0].z += b.z;
		accumulatedBitan[i1].x += b.x; accumulatedBitan[i1].y += b.y; accumulatedBitan[i1].z += b.z;
		accumulatedBitan[i2].x += b.x; accumulatedBitan[i2].y += b.y; accumulatedBitan[i2].z += b.z;
	}

	for (size_t i = 0; i < vertices.size(); ++i) {
		XMVECTOR N = XMVector3Normalize(XMLoadFloat3(&vertices[i].normal));
		XMVECTOR T = XMLoadFloat3(&accumulatedTan[i]);
		XMVECTOR B = XMLoadFloat3(&accumulatedBitan[i]);

		// Orthonormalize T with respect to N
		T = XMVector3Normalize(XMVectorSubtract(T, XMVectorScale(N, XMVector3Dot(N, T).m128_f32[0])));

		// Calculate handedness
		float handedness = (XMVector3Dot(XMVector3Cross(N, T), B).m128_f32[0] < 0.0f) ? -1.0f : 1.0f;

		XMFLOAT3 tangent;
		XMStoreFloat3(&tangent, T);
		vertices[i].tangent = XMFLOAT4(tangent.x, tangent.y, tangent.z, handedness);
	}
}


void ModelManager::TranslateModel(XMFLOAT3 vec, UINT modelId)
{
	_models[modelId]->Translate(vec);
}

void ModelManager::RotateModel(XMFLOAT3 vec, UINT modelId)
{
	_models[modelId]->Rotate(vec);
}

void ModelManager::ScaleModel(XMFLOAT3 vec, UINT modelId)
{
	_models[modelId]->Scale(vec);
}