#pragma once

#include "pch.h"

#include "GUI.h"
#include "IGUIComponent.h"

class PointLight : public IGUIComponent
{
public:
	PointLight() = default;
	PointLight(float x, float y, float z);
	
	void DrawGUI();
	uint64_t GetID() const { return _id; }

	XMFLOAT3 _position = { 0, 0, 0 };

	float _intensity = 1.0f;
	XMFLOAT3 _color = { 1.0f, 1.0f, 1.0f };

private:
	inline static uint64_t _nextId = 0;
	uint64_t _id = _nextId++;

};
