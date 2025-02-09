#pragma once

#include "pch.h"

#include "GUI.h"

class Camera
{
public:
	Camera() {};
	Camera(XMVECTOR pos, XMVECTOR startUp, FLOAT startYaw, FLOAT startPitch, FLOAT startMoveSpeed, FLOAT startTurnSpeed);

	void ConsumeMouse(FLOAT xChange, FLOAT yChange);
	void ConsumeKey(BOOL* keys, FLOAT deltaTime);

	void DrawGUI();

	XMFLOAT4X4 GetViewMatrix() { return _viewMatrix; };
	FLOAT _yaw;
	FLOAT _pitch;
	XMVECTOR _position;

private:
	void Update();

	XMVECTOR _front;
	XMVECTOR _up;
	XMVECTOR _right;
	XMVECTOR _worldUp;

	XMFLOAT4X4 _viewMatrix;

	FLOAT _moveSpeed;
	FLOAT _turnSpeed;
};
