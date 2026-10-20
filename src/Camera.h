#pragma once

#include "pch.h"

#include "GUI.h"
#include "IGUIComponent.h"

class Camera : public IGUIComponent
{
public:
	Camera() = default;
	Camera(XMVECTOR pos, XMVECTOR startUp, float startYaw, float startPitch, float startMoveSpeed, float startTurnSpeed);

	void ConsumeMouse(float xChange, float yChange);
	void ConsumeKey(bool* keys, float deltaTime);
	void Update();

	void DrawGUI();
	uint64_t GetID() const { return _id; }

	XMFLOAT4X4 GetViewMatrix() { return _viewMatrix; };
	float _yaw;
	float _pitch;
	XMVECTOR _position;

private:
	inline static uint64_t _nextId = 0;

	XMVECTOR _front;
	XMVECTOR _up;
	XMVECTOR _right;
	XMVECTOR _worldUp;

	XMFLOAT4X4 _viewMatrix;

	float _moveSpeed;
	float _turnSpeed;

	float _multiplier = 10.0f;
	XMFLOAT3 _uiposition;
	uint64_t _id = _nextId++;

};
