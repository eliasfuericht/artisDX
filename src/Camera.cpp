#include "Camera.h"

Camera::Camera(XMVECTOR pos, XMVECTOR startUp, FLOAT startYaw, FLOAT startPitch, FLOAT startMoveSpeed, FLOAT startTurnSpeed)
	: _position(pos), _worldUp(startUp), _yaw(startYaw), _pitch(startPitch), _moveSpeed(startMoveSpeed), _turnSpeed(startTurnSpeed)
{
	_front = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
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
		_position = XMVectorAdd(_position, XMVectorScale(_front, velocity));
	}
	if (keys[KEYCODES::A])
	{
		_position = XMVectorAdd(_position, XMVectorScale(_right, velocity));
	}
	if (keys[KEYCODES::S])
	{
		_position = XMVectorSubtract(_position, XMVectorScale(_front, velocity));
	}
	if (keys[KEYCODES::D])
	{
		_position = XMVectorSubtract(_position, XMVectorScale(_right, velocity));
	}
	if (keys[KEYCODES::SPACE])
	{
		_position = XMVectorAdd(_position, XMVectorScale(_up, velocity));
	}
	if (keys[KEYCODES::LCTRL])
	{
		_position = XMVectorSubtract(_position, XMVectorScale(_up, velocity));
	}

}

#include <DirectXMath.h>

void Camera::Update()
{
	using namespace DirectX;

	// Convert angles to radians
	float yawRad = XMConvertToRadians(_yaw);
	float pitchRad = XMConvertToRadians(_pitch);

	// Compute front vector
	_front = XMVector3Normalize(XMVectorSet(
		cosf(yawRad) * cosf(pitchRad),
		sinf(pitchRad),
		-sinf(yawRad) * cosf(pitchRad),
		0.0f
	));

	// Compute right and up vectors
	_right = XMVector3Normalize(XMVector3Cross(_front, _worldUp));
	_up = XMVector3Normalize(XMVector3Cross(_right, _front));

	// Compute view matrix
	XMStoreFloat4x4(&_viewMatrix, XMMatrixLookAtLH(_position, XMVectorAdd(_position, _front), _up));
}
