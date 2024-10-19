#include "Camera.h"

Camera::Camera(DirectX::XMVECTOR pos, DirectX::XMVECTOR startUp, FLOAT startYaw, FLOAT startPitch, FLOAT startMoveSpeed, FLOAT startTurnSpeed) 
	: _position(pos), _worldUp(startUp), _yaw(startYaw), _pitch(startPitch), _moveSpeed(startMoveSpeed), _turnSpeed(startTurnSpeed)
{
	_front = DirectX::XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
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
	FLOAT velocity = _moveSpeed * deltaTime;

	if (keys[KEYCODES::W])
	{
		_position = DirectX::XMVectorAdd(_position, DirectX::XMVectorScale(_front, velocity));
	}
	if (keys[KEYCODES::A])
	{
		_position = DirectX::XMVectorAdd(_position, DirectX::XMVectorScale(_right, velocity));
	}
	if (keys[KEYCODES::S])
	{
		_position = DirectX::XMVectorSubtract(_position, DirectX::XMVectorScale(_front, velocity));
	}
	if (keys[KEYCODES::D])
	{
		_position = DirectX::XMVectorSubtract(_position, DirectX::XMVectorScale(_right, velocity));
	}
	if (keys[KEYCODES::SPACE])
	{
		_position = DirectX::XMVectorAdd(_position, DirectX::XMVectorScale(_up, velocity));
	}
	if (keys[KEYCODES::LCTRL])
	{
		_position = DirectX::XMVectorSubtract(_position, DirectX::XMVectorScale(_up, velocity));
	}
}

void Camera::Update()
{
	_front = DirectX::XMVectorSet(
		DirectX::XMScalarCos(DirectX::XMConvertToRadians(_yaw)) * DirectX::XMScalarCos(DirectX::XMConvertToRadians(_pitch)),
		DirectX::XMScalarSin(DirectX::XMConvertToRadians(_pitch)),
		DirectX::XMScalarSin(DirectX::XMConvertToRadians(_yaw)) * DirectX::XMScalarCos(DirectX::XMConvertToRadians(_pitch)),
		0.0f
	);
	_front = DirectX::XMVector3Normalize(_front);

	_right = DirectX::XMVector3Cross(_front, _worldUp);
	_right = DirectX::XMVector3Normalize(_right);

	_up = DirectX::XMVector3Cross(_right, _front);
	_up = DirectX::XMVector3Normalize(_up);

	_viewMatrix = DirectX::XMMatrixLookAtLH(_position, DirectX::XMVectorAdd(_position, _front), _up);
}
