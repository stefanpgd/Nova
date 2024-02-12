#pragma once

#include <string>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXRootSignature;

class DXPipeline
{
public:
	DXPipeline(const std::string& vertexPath, const std::string pixelPath, DXRootSignature* rootsignature, 
		bool doAlphaBlending = false, bool usePixelShader = true, bool doBackCull = false);

	ComPtr<ID3D12PipelineState> Get();
	ID3D12PipelineState* GetAddress();

private:
	void CompileShaders(const std::string& vertexPath, const std::string pixelPath);
	void CreatePipelineState(DXRootSignature* rootsignature);

private:
	ComPtr<ID3D12PipelineState> pipeline;

	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;

	bool alphaBlending;
	bool usePixelShader; 
	bool doBackCull;
};