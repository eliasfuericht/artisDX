#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"

#include "GUI.h"
#include "IGUIComponent.h"

class PointLight : public IGUIComponent
{
public:
	PointLight() {};
	PointLight(float x, float y, float z);
	
	void UpdateBuffer();
	void DrawGUI();

	XMFLOAT3 _position = { 0, 0, 0 };

	uint8_t* _mappedCBVpLightPtr = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE _cbvpLightCPUHandle = {};

private:
	void CreateCBV();

	MSWRL::ComPtr<ID3D12Resource> _pLightBufferResource;
};