#pragma once

#include "Graphics/RenderStage.h"
#include <glm.hpp>

class Scene;
class Mesh;
class Texture;

class SkydomeStage : public RenderStage
{
public:
	SkydomeStage(Window* window, Scene* scene);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;
	void SetScene(Scene* newScene);

private:
	void CreatePipeline();

private:
	Scene* scene;
	Texture* skydomeTexture;
	Mesh* skydomeMesh;

	glm::mat4 skydomeMatrix;
};