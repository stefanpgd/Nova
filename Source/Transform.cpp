#include "Transform.h"

Transform::Transform()
{
	model = XMMatrixIdentity();

	Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
}

const matrix& Transform::GetModelMatrix()
{
	// TODO: Replace with GLM
	matrix translation = XMMatrixTranslation(Position.x, Position.y, Position.x);

	float rotX = XMConvertToRadians(Rotation.x);
	float rotY = XMConvertToRadians(Rotation.y);
	float rotZ = XMConvertToRadians(Rotation.z);
	matrix rotation = XMMatrixRotationRollPitchYaw(rotX, rotY, rotZ);

	matrix scaling = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

	model = XMMatrixIdentity();
	model = XMMatrixMultiply(model, translation);
	model = XMMatrixMultiply(model, rotation);
	model = XMMatrixMultiply(model, scaling);

	return model;
}