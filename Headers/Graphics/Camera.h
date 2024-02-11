#pragma once
#include "Framework/Mathematics.h"

class Camera
{
public: 
	Camera();
	Camera(int windowWidth, int windowHeight);

	void Update(float deltaTime);

	void UpdateViewMatrix();
	void ResizeProjectionMatrix(int windowWidth, int windowHeight);

	const glm::vec3& GetForwardVector();
	const glm::vec3& GetUpwardVector();

	const glm::mat4& GetViewMatrix();
	const glm::mat4& GetProjectionMatrix();
	const glm::mat4& GetViewProjectionMatrix();

public:
	glm::vec3 Position;

private:
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 viewProjection; // stored locally for data copying

	glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	float FOV = 55.0f;
	float nearClip = 0.01f;
	float farClip = 1000.0f;
	float aspectRatio;

	float time = 0.0f;

	float speed = 12.0f;
	float speedMultiplier = 6.0f;
};