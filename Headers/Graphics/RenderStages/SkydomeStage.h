#pragma once

#include "Graphics/RenderStage.h"

class Mesh;
class Camera;
class Texture;

class SkydomeStage : public RenderStage
{
public:
	SkydomeStage(Window* window, Camera* camera);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;

private:
	void CreatePipeline();

private:
	Camera* camera;
	Texture* skydomeTexture;
	Mesh* skydomeMesh;
};