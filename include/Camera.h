#pragma once

#include "pch.h"

class Camera
{
public:
	Camera() {};
	Camera(DirectX::XMVECTOR pos, DirectX::XMVECTOR startUp, FLOAT startYaw, FLOAT startPitch, FLOAT startMoveSpeed, FLOAT startTurnSpeed);

	void ConsumeMouse(FLOAT xChange, FLOAT yChange);
	void ConsumeKey(BOOL* keys, FLOAT deltaTime);

	DirectX::XMMATRIX GetViewMatrix() { return _viewMatrix; };

private:
	void Update();

	DirectX::XMVECTOR _position;
	DirectX::XMVECTOR _front;
	DirectX::XMVECTOR _up;
	DirectX::XMVECTOR _right;
	DirectX::XMVECTOR _worldUp;

	FLOAT _yaw;
	FLOAT _pitch;

	DirectX::XMMATRIX _viewMatrix;

	FLOAT _moveSpeed;
	FLOAT _turnSpeed;
};
