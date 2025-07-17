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
	DirectionalLight(float x, float y, float z, float enableShadowMap, int32_t shadowMapResolution);

	void UpdateBuffer();
	void DrawGUI();

	XMFLOAT3 _direction = { 0, 0, 0 };

	uint8_t* _mappedDirectionPtr = nullptr;
	uint8_t* _mappedLVPPtr = nullptr;
	uint8_t* _mappedSTPtr = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE _dLightDirectionCPUHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE _dLightLVPCPUHandle = {};
	D3D12_CPU_DESCRIPTOR_HANDLE _dLightSTCPUHandle = {};

	XMFLOAT4X4 _lightViewProjMatrix;
	XMFLOAT4X4 _shadowTransformMatrix;

	MSWRL::ComPtr<ID3D12DescriptorHeap> _directionalShadowMapHeap;
	MSWRL::ComPtr<ID3D12Resource> _directionalShadowMapBuffer;

private:
	void BuildLightProjMatrix();
	void CreateShadowMapResource(int32_t resolution);
	void CreateCBV(unsigned long long size, D3D12_CPU_DESCRIPTOR_HANDLE& handle, MSWRL::ComPtr<ID3D12Resource>& buffer, uint8_t*& mappedPtr);

	MSWRL::ComPtr<ID3D12Resource> _dLightDirectionBufferResource;
	MSWRL::ComPtr<ID3D12Resource> _dLightLVPBufferResource;
	MSWRL::ComPtr<ID3D12Resource> _dLightSTBufferResource;
};