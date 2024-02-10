#pragma once
#include "Graphics/RenderStage.h"

#include <vector>

class Scene;

class SceneStage : public RenderStage
{
public:
	SceneStage(Window* window, Scene* scene);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;
	void SetScene(Scene* newScene);

private:
	void CreatePipeline();

private:
	Scene* scene;
};