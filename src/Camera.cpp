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

void Camera::DrawGUI()
{
	GUI::Begin("Camera Window");

	XMFLOAT3 temp = { 0.0f, 0.0f, 0.0f };

	GUI::SliderFloat3("position", temp, -100.0f, 100.0f);

	GUI::End();
}

void Camera::ConsumeKey(BOOL* keys, FLOAT deltaTime)
{
	FLOAT multiplier = 1.0f;
	if (keys[KEYCODES::SHIFT])
	{
		multiplier = 25.0f;
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

	Update();
}

void Camera::Update()
{
	float yawRad = XMConvertToRadians(_yaw);
	float pitchRad = XMConvertToRadians(_pitch);

	_front = XMVector3Normalize(XMVectorSet(
		cosf(yawRad) * cosf(pitchRad),
		sinf(pitchRad),
		-sinf(yawRad) * cosf(pitchRad),
		0.0f
	));

	_right = XMVector3Normalize(XMVector3Cross(_front, _worldUp));
	_up = XMVector3Normalize(XMVector3Cross(_right, _front));

	XMStoreFloat4x4(&_viewMatrix, XMMatrixLookAtLH(_position, XMVectorAdd(_position, _front), _up));
}
