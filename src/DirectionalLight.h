#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"

#include "GUI.h"
#include "IGUIComponent.h"

class DirectionalLight : public IGUIComponent
{
public:
	DirectionalLight() = default;
	DirectionalLight(float x, float y, float z, float enableShadowMap);

	void UpdateBuffer();
	void DrawGUI();
	uint64_t GetID() const { return _id; }

	XMFLOAT3 _direction = { 0.0f, 0.0f, 0.0f };
	bool _shadowsEnabled = false;
	float _intensity = 1.0f;
	XMFLOAT3 _color = { 1.0f, 1.0f, 1.0f };
	uint32_t _shadowMapIndex = 0;
	XMFLOAT4X4 _lightViewProjMatrix;
	
	float _orthoWidth = 40.0f;
	float _orthoHeight = 40.0f;
	float _nearPlane = -20.0f;
	float _farPlane = 10.0f;

private:
	inline static uint64_t _nextId = 0;
	uint64_t _id = _nextId++;

	void BuildLightProjMatrix();
};
