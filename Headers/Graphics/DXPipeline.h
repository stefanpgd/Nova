#pragma once

#include <string>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXRootSignature;

struct DXPipelineDescription
{
	DXRootSignature* RootSignature;

	std::string VertexPath;
	std::string PixelPath;

	DXGI_FORMAT RenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	bool UsePixelShader = true;
	bool DoAlphaBlending = false;
	bool DoBackCulling = false;
};

class DXPipeline
{
public:
	DXPipeline(const DXPipelineDescription& pipelineDescription);

	ComPtr<ID3D12PipelineState> Get();
	ID3D12PipelineState* GetAddress();

private:
	void CompileShaders();
	void CreatePipelineState();

private:
	ComPtr<ID3D12PipelineState> pipeline;

	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;

	DXPipelineDescription description;
};