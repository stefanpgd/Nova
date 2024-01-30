#pragma once
#include <glm.hpp>

const int MAX_AMOUNT_OF_LIGHTS = 15;

// Memory aligned lighting structs // 
struct PointLight
{
	glm::vec3 Position = glm::vec3(0.0f);	// 00 - 12 //
	float Intensity = 10.0f;				// 12 - 16 //
	glm::vec4 Color = glm::vec4(1.0f);		// 16 - 32 //
};

struct LightData
{
	PointLight pointLights[15]; // 000 - 480 //
	int activePointLights;		// 480 - 484 //
	glm::vec3 stub;				// 484 - 496 //
	glm::vec4 stub2;			// 496 - 512 // 
};