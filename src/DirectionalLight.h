#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"

#include "GUI.h"
#include "IGUIComponent.h"

class DirectionalLight : public IGUIComponent
{
public:
	DirectionalLight() {};
	DirectionalLight(float x, float y, float z);

	void UpdateBuffer();
	void DrawGUI();

	XMFLOAT3 _direction = { 0, 0, 0 };

	uint8_t* _mappedCBVdLightPtr = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE _cbvdLightCPUHandle = {};

private:
	void CreateCBV();

	MSWRL::ComPtr<ID3D12Resource> _dLightBufferResource;
};