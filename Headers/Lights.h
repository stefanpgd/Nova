#pragma once
#include <glm.hpp>

// Memory aligned lighting structs // 

struct PointLight
{
	glm::vec3 Position;		// 00 - 12 //
	float Intensity = 1.0f; // 12 - 16 //
	glm::vec4 Color;		// 16 - 32 //
};

struct LightData
{
	PointLight pointLights[15]; // 000 - 480 //
	int activePointLights;		// 480 - 484 //
	glm::vec3 stub;				// 484 - 496 //
	glm::vec4 stub2;			// 496 - 512 // 
};