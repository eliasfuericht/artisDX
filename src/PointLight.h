#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"

#include "GUI.h"
#include "IGUIComponent.h"

class PointLight : public IGUIComponent
{
public:
	PointLight() = default;
	PointLight(float x, float y, float z);
	
	void UpdateBuffer();
	void DrawGUI();
	uint64_t GetID() const { return _id; }

	XMFLOAT3 _position = { 0, 0, 0 };

	uint8_t* _mappedCBVpLightPtr = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE _cbvpLightCPUHandle = {};

private:
	inline static uint64_t _nextId = 0;
	uint64_t _id = _nextId++;

	void CreateCBV();

	MSWRL::ComPtr<ID3D12Resource> _pLightBufferResource;
};
