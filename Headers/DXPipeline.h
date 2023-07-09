#pragma once

#include <string>
#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>

using namespace Microsoft::WRL;

class DXPipeline
{
public:
	DXPipeline(std::wstring vertexShader, std::wstring pixelShader, CD3DX12_ROOT_PARAMETER1* rootParameters, unsigned int numberOfParameters);

	void Set();

private:
	void SetupShaders(std::wstring vertexShader, std::wstring pixelShader);
	void SetupRootsignature(CD3DX12_ROOT_PARAMETER1* rootParameters, unsigned int numberOfParameters);
	void SetupPipelineState();

	ComPtr<ID3DBlob> vertexShaderBlob;
	ComPtr<ID3DBlob> pixelShaderBlob;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
};