#pragma once

#include "Graphics/RenderStage.h"
#include <glm.hpp>
#include <d3dx12.h>

class Scene;
class Mesh;
class Texture;
class HDRI;

class SkydomeStage : public RenderStage
{
public:
	SkydomeStage(Window* window, Scene* scene);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;
	void SetScene(Scene* newScene);

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSkydomeHandle();
	HDRI* GetHDRI();

private:
	void CreatePipeline();

private:
	Scene* scene;
	Texture* skydomeTexture;
	Mesh* skydomeMesh;

	glm::mat4 skydomeMatrix;
};