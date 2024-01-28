#include "DXPipeline.h"
#include <d3dcompiler.h>
#include <cassert>

#include "d3dx12.h"
#include "DXUtilities.h"
#include "DXAccess.h"
#include "DXRootSignature.h"
#include "Logger.h"

DXPipeline::DXPipeline(const std::string& vertexPath, const std::string pixelPath, DXRootSignature* rootSignature)
{
	CompileShaders(vertexPath, pixelPath);
	CreatePipelineState(rootSignature);
}

ComPtr<ID3D12PipelineState> DXPipeline::Get()
{
	return pipeline;
}

ID3D12PipelineState* DXPipeline::GetAddress()
{
	return pipeline.Get();
}

void DXPipeline::CompileShaders(const std::string& vertexPath, const std::string pixelPath)
{
	// Vertex Shader //
	ComPtr<ID3DBlob> vertexError;
	std::wstring vertexShaderPath(vertexPath.begin(), vertexPath.end());

	D3DCompileFromFile(vertexShaderPath.c_str(), NULL, NULL, "main", "vs_5_1", 0, 0, &vertexShaderBlob, &vertexError);

	if(!vertexError == NULL)
	{
		std::string buffer = std::string((char*)vertexError->GetBufferPointer());
		LOG(Log::MessageType::Error, buffer);
		assert(false && "Compilation of shader failed, read console for errors.");
	}

	// Pixel Shader //
	ComPtr<ID3DBlob> pixelError;
	std::wstring pixelShaderPath(pixelPath.begin(), pixelPath.end());

	D3DCompileFromFile(pixelShaderPath.c_str(), NULL, NULL, "main", "ps_5_1", 0, 0, &pixelShaderBlob, &pixelError);

	if(!pixelError == NULL)
	{
		std::string buffer = std::string((char*)pixelError->GetBufferPointer());
		LOG(Log::MessageType::Error, buffer);
		assert(false && "Compilation of shader failed, read console for errors.");
	}
}

void DXPipeline::CreatePipelineState(DXRootSignature* rootSignature)
{
	// Input Layout //
	// input layouts describe to the Input Assembler what the layout of the vertex buffer is
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// Any object in the StateStream is considered a 'Token'
	// Any token in this struct will automatically be implemented in the PSO
	// For example: If the token 'Stream Output' isn't present, it won't be compiled with the PSO.
	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
	} PSS;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	PSS.RootSignature = rootSignature->Get().Get();
	PSS.InputLayout = { inputLayout, _countof(inputLayout) };
	PSS.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	PSS.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	PSS.RTVFormats = rtvFormats;
	PSS.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	PSS.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());

	D3D12_PIPELINE_STATE_STREAM_DESC pssDescription = { sizeof(PSS), &PSS };
	ThrowIfFailed(DXAccess::GetDevice()->CreatePipelineState(&pssDescription, IID_PPV_ARGS(&pipeline)));
}