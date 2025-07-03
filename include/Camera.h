#pragma once

#include "precompiled/pch.h"

#include "GUI.h"
#include "IGUIComponent.h"

class Camera : public IGUIComponent
{
public:
	Camera() {};
	Camera(XMVECTOR pos, XMVECTOR startUp, FLOAT startYaw, FLOAT startPitch, FLOAT startMoveSpeed, FLOAT startTurnSpeed);

	void ConsumeMouse(FLOAT xChange, FLOAT yChange);
	void ConsumeKey(BOOL* keys, FLOAT deltaTime);
	void Update();

	void DrawGUI();
	void RegisterWithGUI();

	XMFLOAT4X4 GetViewMatrix() { return _viewMatrix; };
	FLOAT _yaw;
	FLOAT _pitch;
	XMVECTOR _position;

private:

	XMVECTOR _front;
	XMVECTOR _up;
	XMVECTOR _right;
	XMVECTOR _worldUp;

	XMFLOAT4X4 _viewMatrix;

	FLOAT _moveSpeed;
	FLOAT _turnSpeed;

	FLOAT _multiplier = 25.0f;
};
