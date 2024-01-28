#pragma once

#include "DXUtilities.h"

class Camera
{
public: 
	Camera(int windowWidth, int windowHeight);

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
};