#include "Transform.h"

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

Transform::Transform()
{
	model = glm::mat4(1.0f);
}

const glm::mat4& Transform::GetModelMatrix()
{
	model = glm::mat4(1.0f);
	model = glm::translate(model, Position);
	
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
	model *= rotationMatrix;

	model = glm::scale(model, Scale);

	return model;
}