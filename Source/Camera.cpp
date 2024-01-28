#include "Camera.h"
#include "Input.h"

Camera::Camera(int windowWidth, int windowHeight)
{
	UpdateViewMatrix();
	ResizeProjectionMatrix(windowWidth, windowHeight);

	position = float3(0.0f, 0.0f, 25.0f);
}

void Camera::Update(float deltaTime)
{
	float movement = speed * deltaTime;

	if(Input::GetKey(KeyCode::Shift))
	{
		movement *= speedMultiplier;
	}

	if(Input::GetKey(KeyCode::Ctrl))
	{
		movement /= speedMultiplier;
	}

	int right = Input::GetKey(KeyCode::A) - Input::GetKey(KeyCode::D);
	int up = Input::GetKey(KeyCode::E) - Input::GetKey(KeyCode::Q);
	int forward = Input::GetKey(KeyCode::S) - Input::GetKey(KeyCode::W);

	position.x += right * movement;
	position.y += up * movement;
	position.z += forward * movement;

	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	// Hardcoded for now, replace with input vars... 
	const XMVECTOR eyePosition = XMVectorSet(position.x, position.y, position.z, 1);
	const XMVECTOR focusPoint = XMVector4Normalize(XMVectorSet(position.x, position.y, position.z + 1, 1));
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	view = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);
}

void Camera::ResizeProjectionMatrix(int windowWidth, int windowHeight)
{
	aspectRatio = float(windowWidth) / float(windowHeight);
	projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(FOV), aspectRatio, 0.01f, 1000.0f);
}

matrix Camera::GetViewProjectionMatrix()
{
	return XMMatrixMultiply(view, projection);
}

const matrix& Camera::GetViewMatrix()
{
	return view;
}

const matrix& Camera::GetProjectionMatrix()
{
	return projection;
}