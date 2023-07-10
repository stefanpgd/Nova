#include "DXPipeline.h"
#include "DXUtilities.h"
#include "DXAccess.h"
#include "DXCommands.h"
#include "DXDevice.h"

#include <d3dx12.h>
#include <d3dcompiler.h>

DXPipeline::DXPipeline(std::wstring vertexShader, std::wstring pixelShader, CD3DX12_ROOT_PARAMETER1* rootParameters, unsigned int numberOfParameters)
{
	SetupShaders(vertexShader, pixelShader);
	SetupRootsignature(rootParameters, numberOfParameters);
	SetupPipelineState();
}

void DXPipeline::Set()
{
	ComPtr<ID3D12GraphicsCommandList2> commandList = DXAccess::GetCommands()->GetCommandList();

	commandList->SetPipelineState(pipelineState.Get());
	commandList->SetGraphicsRootSignature(rootSignature.Get());
}

void DXPipeline::SetupShaders(std::wstring vertexShader, std::wstring pixelShader)
{
	UINT compileFlags = 0;

#if defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// Replace with AssetPath once we've the util
	std::wstring vertexPath = L"Shaders\\" + vertexShader;
	std::wstring pixelPath = L"Shaders\\" + pixelShader;

	ThrowIfFailed(D3DCompileFromFile(vertexPath.c_str(), nullptr, nullptr, "main", "vs_5_1", compileFlags, 0, &vertexShaderBlob, nullptr));
	ThrowIfFailed(D3DCompileFromFile(pixelPath.c_str(), nullptr, nullptr, "main", "ps_5_1", compileFlags, 0, &pixelShaderBlob, nullptr));
}

void DXPipeline::SetupRootsignature(CD3DX12_ROOT_PARAMETER1* rootParameters, unsigned int numberOfParameters)
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(numberOfParameters, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

	ThrowIfFailed(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void DXPipeline::SetupPipelineState()
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER rasterizer;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineDesc;

	CD3DX12_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_FRONT;

	pipelineDesc.pRootSignature = rootSignature.Get();
	pipelineDesc.rasterizer = rasterizerDesc;
	pipelineDesc.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineDesc.RTVFormats = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineDesc
	};

	ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipelineState)));
}