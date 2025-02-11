#pragma once

#include "pch.h"

#include "Mesh.h"
#include "AABB.h"
#include "GUI.h"

class Model
{
public:
	Model() {};
	Model(INT id, MSWRL::ComPtr<ID3D12Device> device, std::vector<Vertex> vertices, std::vector<uint32_t> indices, XMFLOAT4X4 modelMatrix);
	void DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void DrawModelGUI();

	INT GetID();
	AABB GetAABB();

	void Translate(XMFLOAT3 vec);
	void Rotate(XMFLOAT3 vec);
	void Scale(XMFLOAT3 vec);


private:
	void CreateModelMatrixBuffer(MSWRL::ComPtr<ID3D12Device> device);
	void ExtractTransformsFromMatrix();
	void UpdateModelMatrix();

	INT _ID;

	Mesh _mesh;
	AABB _aabb;

	XMFLOAT3 _translation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 _rotation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 _scaling = { 1.0f, 1.0f, 1.0f };
	XMFLOAT4X4 _modelMatrix;

	MSWRL::ComPtr<ID3D12Resource> _modelMatrixBuffer;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _modelMatrixBufferHeap;

	UINT8* _mappedUniformBuffer;
};