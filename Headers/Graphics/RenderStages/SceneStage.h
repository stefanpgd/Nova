#pragma once
#include "Graphics/RenderStage.h"

#include <vector>
#include <glm.hpp>
#include <d3dx12.h>

class Scene;
class ShadowStage;

class SceneStage : public RenderStage
{
public:
	SceneStage(Window* window, Scene* scene, ShadowStage* shadowStage);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;
	void SetScene(Scene* newScene);

	void SetSkydome(CD3DX12_GPU_DESCRIPTOR_HANDLE skydomeHandle);

private:
	void CreatePipeline();

private:
	ShadowStage* shadowStage;

	Scene* scene;
	CD3DX12_GPU_DESCRIPTOR_HANDLE skydomeHandle;
};