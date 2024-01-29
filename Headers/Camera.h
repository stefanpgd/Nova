#pragma once
#include "Mathematics.h"

class Camera
{
public: 
	Camera(int windowWidth, int windowHeight);

	void Update(float deltaTime);

	void UpdateViewMatrix();
	void ResizeProjectionMatrix(int windowWidth, int windowHeight);

	const glm::mat4& GetViewProjectionMatrix();

	const glm::mat4& GetViewMatrix();
	const glm::mat4& GetProjectionMatrix();

private:
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 viewProjection; // stored locally for data copying

	float FOV = 45.0f;
	float nearClip = 0.01f;
	float farClip = 1000.0f;
	float aspectRatio;

	glm::vec3 position;
	float speed = 12.0f;
	float speedMultiplier = 6.0f;
};