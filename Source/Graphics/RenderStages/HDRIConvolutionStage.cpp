#include "Graphics/RenderStages/HDRIConvolutionStage.h"
#include "Graphics/HDRI.h"

#include "Graphics/DXRootSignature.h"
#include "Graphics/DXPipeline.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DepthBuffer.h"
#include "Graphics/Mesh.h"


HDRIConvolutionStage::HDRIConvolutionStage(Window* window) 
	: RenderStage(window)
{
	CreatePipeline();
	CreateScreenMesh();
}

void HDRIConvolutionStage::RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	if(!activeHDRI)
	{
		return;
	}

	if(activeHDRI->IsConvoluted)
	{
		return;
	}

	// 0. Grab all relevant variables //
	ComPtr<ID3D12Resource> irradianceResource = activeHDRI->GetIrradianceResource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE hdriTexture = activeHDRI->GetHDRISRVHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE renderTarget = activeHDRI->GetIraddianceRTVHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthHandle = depthBuffer->GetDSV();
	
	// transition to render target
	TransitionResource(irradianceResource.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	
	TransitionResource(depthBuffer->GetResource().Get(),
	D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// 1. Bind pipeline & root //
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	commandList->SetPipelineState(pipeline->GetAddress());
	
	// bind viewport, rect, rendertarget
	commandList->ClearDepthStencilView(depthHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->OMSetRenderTargets(1, &renderTarget, FALSE, &depthHandle);
	
	// Bind shader requirements
	commandList->SetGraphicsRootDescriptorTable(0, hdriTexture);
	
	// Bind mesh & draw 
	commandList->IASetVertexBuffers(0, 1, &screenMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&screenMesh->GetIndexBufferView());
	commandList->DrawIndexedInstanced(screenMesh->GetIndicesCount(), 1, 0, 0, 0);
	
	// Transition back to shader resource
	TransitionResource(irradianceResource.Get(),
	D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
	activeHDRI->IsConvoluted = true;
}

void HDRIConvolutionStage::BindHDRI(HDRI* hdri)
{
	activeHDRI = hdri;

	scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
		static_cast<float>(hdri->GetWidth()), static_cast<float>(hdri->GetHeight()));

	depthBuffer = new DepthBuffer(hdri->GetWidth(), hdri->GetHeight());
}

void HDRIConvolutionStage::CreatePipeline()
{
	CD3DX12_DESCRIPTOR_RANGE1 hdriRange[1];
	hdriRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // HDRI 

	CD3DX12_ROOT_PARAMETER1 skydomeRootParameters[1];
	skydomeRootParameters[0].InitAsDescriptorTable(1, &hdriRange[0], D3D12_SHADER_VISIBILITY_PIXEL); // Skydome Texture

	rootSignature = new DXRootSignature(skydomeRootParameters, _countof(skydomeRootParameters),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	pipeline = new DXPipeline("Source/Shaders/hdriConvolution.vertex.hlsl",
		"Source/Shaders/hdriConvolution.pixel.hlsl", rootSignature, false, true, false, DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void HDRIConvolutionStage::CreateScreenMesh()
{
	Vertex* screenVertices = new Vertex[4];
	screenVertices[0].Position = glm::vec3(-1.0f, -1.0f, 0.0f);
	screenVertices[1].Position = glm::vec3(-1.0f, 1.0f, 0.0f);
	screenVertices[2].Position = glm::vec3(1.0f, 1.0f, 0.0f);
	screenVertices[3].Position = glm::vec3(1.0f, -1.0f, 0.0f);

	screenVertices[0].TexCoord = glm::vec2(0.0f, 1.0f);
	screenVertices[1].TexCoord = glm::vec2(0.0f, 0.0f);
	screenVertices[2].TexCoord = glm::vec2(1.0f, 0.0f);
	screenVertices[3].TexCoord = glm::vec2(1.0f, 1.0f);

	unsigned int* screenIndices = new unsigned int[6]
		{	2, 1, 0, 3, 2, 0 };

	screenMesh = new Mesh(screenVertices, 4, screenIndices, 6);

	delete[] screenVertices;
	delete[] screenIndices;
}