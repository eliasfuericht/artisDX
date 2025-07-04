#pragma once 

#include "pch.h"

#include "GraphicsDevice.h"
#include "DescriptorAllocator.h"

class ModelNode {
public:
	ModelNode();

	INT _id;
	std::string _name;
	int _meshIndex = NOTOK;
	int _parentIndex = -1;
	std::vector<int> _children;

	XMFLOAT3 _translation = { 0,0,0 };
	XMFLOAT3 _rotation = { 0,0,0 };
	XMFLOAT3 _scale = { 1,1,1 };

	XMFLOAT4X4 _localMatrix = {}; 
	XMFLOAT4X4 _globalMatrix = {};

	uint8_t* _mappedCBVModelMatrixPtr = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE _cbvModelMatrixGpuHandle = {};

private:
	void CreateCBV();

	MSWRL::ComPtr<ID3D12Resource> _ModelMatrixBufferResource;
};