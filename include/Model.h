#pragma once

#include "precompiled/pch.h"

#include "GUI.h"
#include "IGUIComponent.h"
#include "Primitive.h"
#include "AABB.h"
#include "Texture.h"
#include "Mesh.h"
#include "Material.h"

class Model : public IGUIComponent
{
public:
	Model() {};
	Model(INT id, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<std::vector<Vertex>> meshInstanceVertices, std::vector<std::vector<uint32_t>> meshInstanceIndices,
				std::vector<XMFLOAT4X4> meshInstanceMatrices, std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures, std::vector<std::tuple<INT, std::vector<INT>>> materials);
	
	void DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void DrawGUI();
	void RegisterWithGUI();

	INT GetID();

	void Translate(XMFLOAT3 vec);
	void Rotate(XMFLOAT3 vec);
	void Scale(XMFLOAT3 vec);

	bool _markedForDeletion = false;

private:
	void ExtractTransformsFromMatrix();
	void UpdateTransformMatrix();

	INT _id = NOTOK;
	INT _meshInstanceIdIncrementor = 0;

	XMFLOAT4X4 _transformMatrix;

	XMFLOAT3 _translation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 _rotation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 _scaling = { 1.0f, 1.0f, 1.0f };

	std::vector<Mesh> _meshInstances;
	std::vector<Texture> _textures;
	std::vector<std::vector<INT>> _materialTextureIndices;
};