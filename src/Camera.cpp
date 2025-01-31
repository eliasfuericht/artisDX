#include "Camera.h"

Camera::Camera(glm::vec3 pos, glm::vec3 startUp, FLOAT startYaw, FLOAT startPitch, FLOAT startMoveSpeed, FLOAT startTurnSpeed)
	: _position(pos), _worldUp(startUp), _yaw(startYaw), _pitch(startPitch), _moveSpeed(startMoveSpeed), _turnSpeed(startTurnSpeed)
{
	_front = glm::vec3(0.0f, 0.0f, -1.0f);
}

void Camera::ConsumeMouse(FLOAT xChange, FLOAT yChange)
{
	xChange *= _turnSpeed;
	yChange *= _turnSpeed;

	_yaw += xChange;
	_pitch += yChange;

	if (_pitch > 89.0f)
	{
		_pitch = 89.0f;
	}

	if (_pitch < -89.0f)
	{
		_pitch = -89.0f;
	}

	Update();
}

void Camera::ConsumeKey(BOOL* keys, FLOAT deltaTime)
{
	FLOAT multiplier = 1.0f;
	if (keys[KEYCODES::SHIFT])
	{
		multiplier = 100.0f;
	}

	FLOAT velocity = _moveSpeed * deltaTime * multiplier;

	if (keys[KEYCODES::W])
	{
		_position += _front * velocity;
	}
	if (keys[KEYCODES::A])
	{
		_position -= _right * velocity;
	}
	if (keys[KEYCODES::S])
	{
		_position -= _front * velocity;
	}
	if (keys[KEYCODES::D])
	{
		_position += _right * velocity;
	}
	if (keys[KEYCODES::SPACE])
	{
		_position += _up * velocity;
	}
	if (keys[KEYCODES::LCTRL])
	{
		_position -= _up * velocity;
	}
}

// TODO: fix camera
void Camera::Update()
{
	_front = glm::vec3(
		glm::cos(glm::radians(_yaw)) * glm::cos(glm::radians(_pitch)),
		glm::sin(glm::radians(_pitch)),
		glm::sin(glm::radians(_yaw)) * glm::cos(glm::radians(_pitch))
	);

	_front = glm::normalize(_front);

	_right = glm::cross(_front, _worldUp);
	_right = glm::normalize(_right);

	_up = glm::cross(_right, _front);
	_up = glm::normalize(_up);

	_viewMatrix = glm::lookAt(_position, _position + _front, _up);
}
