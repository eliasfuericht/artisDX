#pragma once

#include "pch.h"

#include "GUI.h"
#include "IGUIComponent.h"
#include "Primitive.h"
#include "AABB.h"
#include "Texture.h"
#include "Mesh.h"
#include "Material.h"
#include "ModelNode.h"

class Model : public IGUIComponent
{
public:
	Model() {};
	Model(INT id, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<Mesh> meshes, std::vector<Texture> textures, std::vector<Material> materials);

	void DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	void DrawGUI();
	INT GetID();

	std::vector<Mesh> _meshes;
	std::vector<Texture> _textures;
	std::vector<Material> _materials;
	std::vector<ModelNode> _modelNodes;

private:
	void ComputeGlobalTransforms();

	INT _id = NOTOK;
};