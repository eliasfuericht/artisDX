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
#include "ShaderPass.h"

class Model : public IGUIComponent
{
public:
	Model() = default;
	Model(int32_t id, std::string name, std::vector<Mesh> meshes, std::vector<Texture> textures, std::vector<Material> materials, std::vector<ModelNode> modelNodes);

	struct DrawItem
	{
		std::weak_ptr<Model> model;
		size_t nodeIndex;
		size_t primitiveIndex;
		fastgltf::AlphaMode alphaMode = fastgltf::AlphaMode::Opaque;
		float viewDepth = 0;
	};

	void ComputeGlobalTransforms();
	void AppendDrawItems(std::vector<Model::DrawItem>& drawItems);
	void UpdateDrawItem(Model::DrawItem& item, const XMFLOAT4X4& viewMatrix) const;
	void DrawPrimitive(const Model::DrawItem& item, std::shared_ptr<ShaderPass> shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void DrawModelBoundingBox(std::shared_ptr<ShaderPass> shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

	void DrawGUI();
	int32_t GetID();

private:
	void ComputeNodeGlobal(int32_t nodeIndex, const XMMATRIX& parentMatrix);

	int32_t _id = NOTOK;
	std::string _name;
	std::vector<Mesh> _meshes;
	std::vector<Texture> _textures;
	std::vector<Material> _materials;
	std::vector<ModelNode> _modelNodes;

	XMFLOAT3 _translation = { 0,0,0 };
	XMFLOAT3 _rotationEuler = { 0,0,0 };
	XMFLOAT3 _scale = { 1,1,1 };

	XMFLOAT4X4 _globalMatrix = {};
};
