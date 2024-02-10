#pragma once
#include "Graphics/RenderStage.h"
#include <d3dx12.h>

class DXPipeline;
class DXRootSignature;
class Mesh;

/// <summary>
/// Screen-spaced pass, uses the render from the SceneStage as a texture
/// </summary>
class ScreenStage : public RenderStage
{
public:
	ScreenStage(Window* window);

	void PlaceHolderFunction(CD3DX12_GPU_DESCRIPTOR_HANDLE handle);
	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;

private:
	void CreatePipeline();
	void CreateScreenMesh();

private:
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle;

	Mesh* screenMesh;
};