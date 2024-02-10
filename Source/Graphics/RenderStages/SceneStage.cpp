#include "Graphics/RenderStages/SceneStage.h"

#include <d3dx12.h>
#include "Graphics/DXRootSignature.h"
#include "Graphics/DXDescriptorHeap.h"
#include "Graphics/DXPipeline.h"
#include "Graphics/Camera.h"
#include "Graphics/DXAccess.h"
#include "Graphics/Model.h"

// TODO: Replace with Scene Reference 
SceneStage::SceneStage(Window* window, Camera* camera, std::vector<Model*>& models, 
	LightData* lights, int lightCBVIndex) : RenderStage(window), camera(camera), models(models), lights(lights), lightDataCBVIndex(lightCBVIndex)
{
	CreatePipeline();
}

void SceneStage::RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	// 0. Grab all relevant objects //
	DXDescriptorHeap* CBVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE lightData = CBVHeap->GetGPUHandleAt(lightDataCBVIndex);

	// 1. Bind pipeline & root //
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	commandList->SetPipelineState(pipeline->GetAddress());

	// 2. Bind root arguments  //
	commandList->SetGraphicsRoot32BitConstants(1, 3, &camera->Position, 0);
	commandList->SetGraphicsRootDescriptorTable(2, lightData);

	// 5. Draw Calls & Bind MVPs ( happens in Model.cpp ) // 
	for(Model* model : models)
	{
		model->Draw(camera->GetViewProjectionMatrix());
	}
}

void SceneStage::CreatePipeline()
{
	CD3DX12_DESCRIPTOR_RANGE1 cbvRanges[1]; // placeholder cause there is no guarantee they are next too each other...
	cbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 1); // Lighting buffer

	CD3DX12_DESCRIPTOR_RANGE1 srvRanges[1];
	srvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // Render Target as Texture

	CD3DX12_ROOT_PARAMETER1 rootParameters[4];
	rootParameters[0].InitAsConstants(32, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // MVP & Model
	rootParameters[1].InitAsConstants(3, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // Scene info ( Camera... etc. ) 
	rootParameters[2].InitAsDescriptorTable(1, &cbvRanges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Lighting data
	rootParameters[3].InitAsDescriptorTable(1, &srvRanges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Textures

	rootSignature = new DXRootSignature(rootParameters, _countof(rootParameters), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	pipeline = new DXPipeline("Source/Shaders/default.vertex.hlsl", "Source/Shaders/default.pixel.hlsl", rootSignature);
}