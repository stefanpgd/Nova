#include "Camera.h"

Camera::Camera(int windowWidth, int windowHeight)
{
	UpdateViewMatrix();
	ResizeProjectionMatrix(windowWidth, windowHeight);
}

void Camera::UpdateViewMatrix()
{
	// Hardcoded for now, replace with input vars... 
	const XMVECTOR eyePosition = XMVectorSet(0, 0, 50, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
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