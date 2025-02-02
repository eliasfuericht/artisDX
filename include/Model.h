#pragma once

#include "pch.h"

#include "Mesh.h"
#include "AABB.h"

class Model
{
public:
	Model() {};
	Model(MSWRL::ComPtr<ID3D12Device> device, std::vector<Vertex> vertices, std::vector<uint32_t> indices);
	void DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);

private:

	Mesh _mesh;
	AABB _aabb;
	DirectX::XMFLOAT4X4 _modelMatrix;
};