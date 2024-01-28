#pragma once

#include "DXUtilities.h"

class Camera
{
public: 
	Camera(int windowWidth, int windowHeight);

	void Update(float deltaTime);

	void UpdateViewMatrix();
	void ResizeProjectionMatrix(int windowWidth, int windowHeight);

	matrix GetViewProjectionMatrix();

	const matrix& GetViewMatrix();
	const matrix& GetProjectionMatrix();

private:
	matrix view;
	matrix projection;

	float FOV = 60.0f;
	float aspectRatio;

	float3 position;
	float speed = 12.0f;
	float speedMultiplier = 6.0f;
};