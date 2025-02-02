#pragma once

#include "pch.h"

class Camera
{
public:
	Camera() {};
	Camera(glm::vec3 pos, glm::vec3 startUp, FLOAT startYaw, FLOAT startPitch, FLOAT startMoveSpeed, FLOAT startTurnSpeed);

	void ConsumeMouse(FLOAT xChange, FLOAT yChange);
	void ConsumeKey(BOOL* keys, FLOAT deltaTime);

	glm::mat4 GetViewMatrix() { return _viewMatrix; };
	FLOAT _yaw;
	FLOAT _pitch;
	glm::vec3 _position;

private:
	void Update();

	glm::vec3 _front;
	glm::vec3 _up;
	glm::vec3 _right;
	glm::vec3 _worldUp;

	glm::mat4 _viewMatrix;
	DirectX::XMFLOAT4X4 _dxViewMatrix;

	FLOAT _moveSpeed;
	FLOAT _turnSpeed;
};
