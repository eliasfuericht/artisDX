#pragma once

#include "pch.h"

class Camera
{
public:
	Camera() {};
	Camera(DirectX::XMVECTOR pos, DirectX::XMVECTOR startUp, FLOAT startYaw, FLOAT startPitch, FLOAT startMoveSpeed, FLOAT startTurnSpeed);

	void ConsumeMouse(FLOAT xChange, FLOAT yChange);
	void ConsumeKey(BOOL* keys, FLOAT deltaTime);

	DirectX::XMFLOAT4X4 GetViewMatrix() { return _viewMatrix; };
	FLOAT _yaw;
	FLOAT _pitch;
	DirectX::XMVECTOR _position;

private:
	void Update();

	DirectX::XMVECTOR _front;
	DirectX::XMVECTOR _up;
	DirectX::XMVECTOR _right;
	DirectX::XMVECTOR _worldUp;

	DirectX::XMFLOAT4X4 _viewMatrix;

	FLOAT _moveSpeed;
	FLOAT _turnSpeed;
};
