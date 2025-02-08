#pragma once

#include "pch.h"

#include "Mesh.h"
#include "AABB.h"
#include "ImGuiRenderer.h"

class Model
{
public:
	Model() {};
	Model(MSWRL::ComPtr<ID3D12Device> device, std::vector<Vertex> vertices, std::vector<uint32_t> indices, DirectX::XMFLOAT4X4 modelMatrix);
	void DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
	void Selected();
	void Translate(DirectX::XMFLOAT3 vec);
	void Rotate(DirectX::XMFLOAT3 vec);
	void Scale(DirectX::XMFLOAT3 vec);


private:
	void CreateModelMatrixBuffer(MSWRL::ComPtr<ID3D12Device> device);

	Mesh _mesh;
	AABB _aabb;
	DirectX::XMFLOAT4X4 _modelMatrix;
	MSWRL::ComPtr<ID3D12Resource> _modelMatrixBuffer;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _modelMatrixBufferHeap;

	UINT8* _mappedUniformBuffer;
};