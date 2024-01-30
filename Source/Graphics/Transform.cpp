#include "Graphics/Transform.h"
#include "Framework/Mathematics.h"

Transform::Transform()
{
	model = glm::mat4(1.0f);

	Position = glm::vec3(0.0f, 0.0f, 0.0f);
	Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	Scale = glm::vec3(1.0f, 1.0f, 1.0f);
}

const glm::mat4& Transform::GetModelMatrix()
{
	model = glm::mat4(1.0f);
	model = glm::translate(model, Position);

#pragma region Rotation Matrix Calculation
	glm::mat4 rotX = glm::mat4(1.0f);
	glm::mat4 rotY = glm::mat4(1.0f);
	glm::mat4 rotZ = glm::mat4(1.0f);

	float radX = glm::radians(Rotation.x);
	float sinX = sinf(radX);
	float cosX = cosf(radX);

	rotX[1][1] = cosX;
	rotX[2][1] = -sinX;
	rotX[1][2] = sinX;
	rotX[2][2] = cosX;

	float radY = glm::radians(Rotation.y);
	float sinY = sinf(radY);
	float cosY = cosf(radY);

	rotY[0][0] = cosY;
	rotY[2][0] = sinY;
	rotY[0][2] = -sinY;
	rotY[2][2] = cosY;

	float radZ = glm::radians(Rotation.z);
	float sinZ = sinf(radZ);
	float cosZ = cosf(radZ);

	rotZ[0][0] = cosZ;
	rotZ[1][0] = -sinZ;
	rotZ[0][1] = sinZ;
	rotZ[1][1] = cosZ;

	glm::mat4 rotationMatrix = rotX * rotY * rotZ;
#pragma endregion

	model *= rotationMatrix;
	model = glm::scale(model, Scale);

	return model;
}

glm::vec3 Transform::GetForwardVector()
{
	glm::mat4 rotate = glm::mat4((glm::quat(glm::radians(Rotation))));
	return glm::vec3(rotate[0][2] * -1, rotate[1][2] * -1, rotate[2][2] * -1);
}

glm::vec3 Transform::GetRightVector()
{
	glm::mat4 rotate = glm::mat4((glm::quat(glm::radians(Rotation))));
	return{ rotate[0][0], rotate[1][0], rotate[2][0] };
}

glm::vec3 Transform::GetUpVector()
{
	glm::mat4 rotate = glm::mat4((glm::quat(glm::radians(Rotation))));
	return{ rotate[0][1], rotate[1][1], rotate[2][1] };
}