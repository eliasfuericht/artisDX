#pragma once 

#include "pch.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"
#include "ShaderPass.h"

class ModelNode {
public:
	ModelNode();
	uint64_t GetID() const { return _id; }

	uint64_t _id = _nextId++;
	std::string _name;
	int32_t _meshIndex = NOTOK;
	int32_t _parentIndex = -1;
	std::vector<int32_t> _children;

	XMFLOAT3 _translation = { 0,0,0 };
	XMFLOAT3 _rotationEuler = { 0,0,0 };
	XMFLOAT4 _rotationQuat = { 0,0,0,1};
	XMFLOAT3 _scale = { 1,1,1 };

	XMFLOAT4X4 _localMatrix = {}; 
	XMFLOAT4X4 _globalMatrix = {};

	uint8_t* _mappedCBVModelMatrixPtr = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE _cbvModelMatrixGpuHandle = {};

	void BindModelMatrixData(std::shared_ptr<ShaderPass> shaderPass, MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList);
private:
	inline static uint64_t _nextId = 0;

	void CreateCBV();

	MSWRL::ComPtr<ID3D12Resource> _ModelMatrixBufferResource;
};
