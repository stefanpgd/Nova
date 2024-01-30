#pragma once
#include "Framework/Mathematics.h"

class Camera
{
public: 
	Camera(int windowWidth, int windowHeight);

	void Update(float deltaTime);

	void UpdateViewMatrix();
	void ResizeProjectionMatrix(int windowWidth, int windowHeight);

	const glm::vec3& GetForwardVector();

	const glm::mat4& GetViewMatrix();
	const glm::mat4& GetProjectionMatrix();
	const glm::mat4& GetViewProjectionMatrix();

public:
	glm::vec3 Position;

private:
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 viewProjection; // stored locally for data copying

	// placeholder till we've look around.
	// Mainly used to test some stuff in shaders atm
	const glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);

	float FOV = 45.0f;
	float nearClip = 0.01f;
	float farClip = 1000.0f;
	float aspectRatio;

	float speed = 12.0f;
	float speedMultiplier = 6.0f;
};