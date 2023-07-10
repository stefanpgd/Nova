#pragma once

#include <glm.hpp>

class Transform
{
public:
	Transform();

	const glm::mat4& GetModelMatrix();

	glm::vec3 Position	{0.0f, 0.0f, 0.0f};
	glm::vec3 Rotation	{0.0f, 0.0f, 0.0f};
	glm::vec3 Scale		{1.0f, 1.0f, 1.0f};

private:
	glm::mat4 model;
};