#pragma once

#include <string>
#include <wrl.h>
#include <d3d12.h>;
using namespace Microsoft::WRL;

class DXRootSignature;

class DXPipeline
{
public:
	DXPipeline(const std::string& vertexPath, const std::string pixelPath, DXRootSignature* rootsignature);

	ComPtr<ID3D12PipelineState> Get();
	ID3D12PipelineState* GetAddress();

private:
	void CompileShaders(const std::string& vertexPath, const std::string pixelPath);
	void CreatePipelineState(DXRootSignature* rootsignature);

private:
	ComPtr<ID3D12PipelineState> pipeline;

	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;
};