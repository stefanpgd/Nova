#include "Graphics/Camera.h"
#include "Framework/Input.h"

Camera::Camera(int windowWidth, int windowHeight)
{
	UpdateViewMatrix();
	ResizeProjectionMatrix(windowWidth, windowHeight);

	Position = glm::vec3(0.0f, 0.0f, 10.0f);
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

	int right = Input::GetKey(KeyCode::D) - Input::GetKey(KeyCode::A);
	int up = Input::GetKey(KeyCode::E) - Input::GetKey(KeyCode::Q);
	int forward = Input::GetKey(KeyCode::S) - Input::GetKey(KeyCode::W);

	Position.x += right * movement;
	Position.y += up * movement;
	Position.z += forward * movement;

	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	// Placeholder until we've a look around
	const glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
	const glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

	view = glm::lookAt(Position, Position + front, up);
}

void Camera::ResizeProjectionMatrix(int windowWidth, int windowHeight)
{
	aspectRatio = float(windowWidth) / float(windowHeight);
	projection = glm::perspective(glm::radians(FOV), aspectRatio, nearClip, farClip);
}

const glm::vec3& Camera::GetForwardVector()
{
	return front;
}

const glm::mat4& Camera::GetViewProjectionMatrix()
{
	viewProjection = projection * view;
	return viewProjection;
}

const glm::mat4& Camera::GetViewMatrix()
{
	return view;
}

const glm::mat4& Camera::GetProjectionMatrix()
{
	return projection;
}