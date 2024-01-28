#pragma once

#include "DXUtilities.h"

class Transform
{
public:
	Transform();

	const matrix& GetModelMatrix();

public:
	float3 Position;
	float3 Rotation;
	float3 Scale;

private:
	matrix model;
};