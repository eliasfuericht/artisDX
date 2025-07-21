#include "Camera.h"

Camera::Camera(XMVECTOR pos, XMVECTOR startUp, float startYaw, float startPitch, float startMoveSpeed, float startTurnSpeed)
	: _position(pos), _worldUp(startUp), _yaw(startYaw), _pitch(startPitch), _moveSpeed(startMoveSpeed), _turnSpeed(startTurnSpeed)
{
	_front = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
}

void Camera::ConsumeMouse(float xChange, float yChange)
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
}

void Camera::ConsumeKey(bool* keys, float deltaTime)
{
	float multiplier = 1.0f;
	if (keys[KEYCODES::KEYCODE_SHIFT])
	{
		multiplier = _multiplier;
	}

	float velocity = _moveSpeed * deltaTime * multiplier;

	if (keys[KEYCODES::KEYCODE_W])
	{
		_position = XMVectorAdd(_position, XMVectorScale(_front, velocity));
	}
	if (keys[KEYCODES::KEYCODE_A])
	{
		_position = XMVectorAdd(_position, XMVectorScale(_right, velocity));
	}
	if (keys[KEYCODES::KEYCODE_S])
	{
		_position = XMVectorSubtract(_position, XMVectorScale(_front, velocity));
	}
	if (keys[KEYCODES::KEYCODE_D])
	{
		_position = XMVectorSubtract(_position, XMVectorScale(_right, velocity));
	}
	if (keys[KEYCODES::KEYCODE_SPACE])
	{
		_position = XMVectorAdd(_position, XMVectorScale(_up, velocity));
	}
	if (keys[KEYCODES::KEYCODE_LCTRL])
	{
		_position = XMVectorSubtract(_position, XMVectorScale(_up, velocity));
	}
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

void Camera::DrawGUI()
{
	ImGui::Begin("Camera Window");

	ImGui::DragFloat("Camera Speed", &_multiplier);

	ImGui::End();
}