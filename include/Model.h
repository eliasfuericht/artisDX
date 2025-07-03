#pragma once

#include "pch.h"

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
	Model(INT id, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<Mesh> meshes, std::vector<Texture> textures, std::vector<Material> materials);

	void DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void DrawGUI();
	void RegisterWithGUI();

	INT GetID();

	void Translate(XMFLOAT3 vec);
	void Rotate(XMFLOAT3 vec);
	void Scale(XMFLOAT3 vec);

	std::vector<Mesh> _meshes;
	std::vector<Texture> _textures;
	std::vector<Material> _materials;

private:
	void ExtractTransformsFromMatrix();
	void UpdateTransformMatrix();

	INT _id = NOTOK;
	INT _meshInstanceIdIncrementor = 0;

	XMFLOAT4X4 _transformMatrix;

	XMFLOAT3 _translation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 _rotation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 _scaling = { 1.0f, 1.0f, 1.0f };

	std::vector<std::vector<INT>> _materialTextureIndices;
};