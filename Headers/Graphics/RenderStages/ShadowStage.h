#pragma once
#include "Graphics/RenderStage.h"
#include <glm.hpp>

class DepthBuffer;
class Scene;

// Shadow stage owns the directional light?
class ShadowStage : public RenderStage
{
public:
	ShadowStage(Window* window, Scene* scene);

	void Update(float deltaTime);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;

	DepthBuffer* GetDepthBuffer();
	const glm::mat4& GetLightMatrix();

private:
	void CreatePipeline();

private:
	float orthoSize = 5.0f;
	float shadowNear = 1.0f;
	float shadowFar = 50.0f;

	unsigned int depthBufferWidth = 4096;
	unsigned int depthBufferHeight = 4096;
	DepthBuffer* depthBuffer;
	
	D3D12_RECT scissorRect;
	D3D12_VIEWPORT viewport;

	Scene* scene;

	glm::vec3 lightPosition;
	glm::vec3 lightDirection;
	glm::vec3 lightTarget;
	glm::mat4 lightMatrix;
};