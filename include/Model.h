#pragma once

#include "precompiled/pch.h"

#include "GUI.h"
#include "IGUIComponent.h"
#include "Mesh.h"
#include "AABB.h"
#include "Texture.h"

class Model : public IGUIComponent
{
public:
	Model() {};
	Model(INT id, MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<Vertex> vertices, std::vector<uint32_t> indices, XMFLOAT4X4 modelMatrix, std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures);
	
	void DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void DrawGUI();
	void RegisterWithGUI();

	INT GetID();
	AABB GetAABB();
	XMFLOAT4X4 GetModelMatrix();

	void Translate(XMFLOAT3 vec);
	void Rotate(XMFLOAT3 vec);
	void Scale(XMFLOAT3 vec);

	bool _markedForDeletion = false;

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

	std::vector<Texture> _textures;

	MSWRL::ComPtr<ID3D12Resource> _modelMatrixBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE _cbvCpuHandle;

	UINT8* _mappedUniformBuffer;
};