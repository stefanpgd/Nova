#pragma once

#include <glm.hpp>

class Transform
{
public:
	Transform();

	const glm::mat4& GetModelMatrix();
	glm::vec3 GetForwardVector();
	glm::vec3 GetRightVector();
	glm::vec3 GetUpVector();

public:
	glm::vec3 Position;
	glm::vec3 Rotation;
	glm::vec3 Scale;

private:
	glm::mat4 model;
};