#pragma once
#include "Graphics/RenderStage.h"
#include <d3d12.h>

class HDRI;
class Mesh;
class DepthBuffer;

class HDRIConvolutionStage : public RenderStage
{
public:
	HDRIConvolutionStage(Window* window);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;

	void BindHDRI(HDRI* hdri);

private:
	void CreatePipeline();
	void CreateScreenMesh();

private:
	Mesh* screenMesh;
	HDRI* activeHDRI = nullptr;

	DepthBuffer* depthBuffer;

	D3D12_RECT scissorRect;
	D3D12_VIEWPORT viewport;
};