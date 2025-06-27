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
	Model(INT id, MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList, std::vector<std::vector<Vertex>> submeshVertices, 
		std::vector<std::vector<uint32_t>> submeshIndices, std::vector<XMFLOAT4X4> submeshMatrices, std::vector<std::tuple<Texture::TEXTURETYPE, ScratchImage>> textures);
	
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
	void UpdateModelMatrix();

	struct SubMesh 
	{
		INT id;
		Mesh mesh;
		AABB aabb;
		XMFLOAT4X4 localTransform;

		XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };
		XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
		XMFLOAT3 scaling = { 1.0f, 1.0f, 1.0f };

		MSWRL::ComPtr<ID3D12Resource> constantBuffer;
		uint8_t* mappedPtr = nullptr;
		D3D12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle = {};

		SubMesh(INT subMeshId, Mesh smesh, AABB saabb, XMFLOAT4X4 slocalTransform, MSWRL::ComPtr<ID3D12Device> device)
		{
			id = subMeshId;
			mesh = smesh;
			aabb = saabb;
			localTransform = slocalTransform;

			CreateCBV(device);
		}
		
		void CreateCBV(MSWRL::ComPtr<ID3D12Device> device)
		{
			D3D12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle = DescriptorAllocator::Instance().Allocate();

			const UINT bufferSize = (sizeof(XMFLOAT4X4) + 255) & ~255;

			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
			CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

			device->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&bufferDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&constantBuffer));

			CD3DX12_RANGE readRange(0, 0);
			constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedPtr));

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = constantBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = bufferSize;

			device->CreateConstantBufferView(&cbvDesc, cbvCpuHandle);

			cbvGpuHandle = DescriptorAllocator::Instance().GetGPUHandle(cbvCpuHandle);
		}
	};

	INT _ID;
	INT _subMeshId = 0;

	std::vector<SubMesh> _subMeshes;
	std::vector<Texture> _textures;

	MSWRL::ComPtr<ID3D12Resource> _modelMatrixBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE _cbvCpuHandle;

	UINT8* _mappedUniformBuffer;
};