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

	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;

private:
	void CreatePipeline();

private:
	float shadowNear = 1.0f;
	float shadowFar = 50.0f;

	unsigned int depthBufferWidth = 1024;
	unsigned int depthBufferHeight = 1024;
	DepthBuffer* depthBuffer;
	
	D3D12_RECT scissorRect;
	D3D12_VIEWPORT viewport;

	Scene* scene;

	glm::vec3 lightPosition;
	glm::vec3 lightDirection;
	glm::mat4 lightMatrix;
};