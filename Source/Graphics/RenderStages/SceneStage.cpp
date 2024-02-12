#include "Graphics/RenderStages/SceneStage.h"

#include "Framework/Scene.h"

#include "Graphics/DXRootSignature.h"
#include "Graphics/DXDescriptorHeap.h"
#include "Graphics/DXPipeline.h"
#include "Graphics/Camera.h"
#include "Graphics/DXAccess.h"
#include "Graphics/Model.h"

#include <imgui_impl_dx12.h>

SceneStage::SceneStage(Window* window, Scene* scene) : RenderStage(window), scene(scene)
{
	CreatePipeline();
}

void SceneStage::RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	// 0. Grab all relevant objects to perform stage //
	DXDescriptorHeap* CBVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DXDescriptorHeap* DSVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	ComPtr<ID3D12Resource> renderBuffer = window->GetCurrentRenderBuffer();

	CD3DX12_CPU_DESCRIPTOR_HANDLE depthView = DSVHeap->GetCPUHandleAt(0);
	CD3DX12_CPU_DESCRIPTOR_HANDLE renderRTV = window->GetCurrentRenderRTV();
	Camera& camera = scene->GetCamera();

	// 1. Transition Render (Texture) to Render Target, afterwards bind target //
	TransitionResource(renderBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	BindAndClearRenderTarget(window, &renderRTV, &depthView);

	// 2. Bind pipeline & root //
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	commandList->SetPipelineState(pipeline->GetAddress());

	// 3. Bind root arguments  //
	commandList->SetGraphicsRoot32BitConstants(1, 3, &camera.Position, 0);
	commandList->SetGraphicsRootDescriptorTable(2, scene->GetLightBufferHandle());
	commandList->SetGraphicsRootDescriptorTable(4, skydomeHandle);

	// 4. Draw Calls & Bind MVPs ( happens in Model.cpp ) // 

	for(Model* model : scene->GetModels())
	{
		model->Draw(camera.GetViewProjectionMatrix());
	}

	// 5. Draw UI/Editor //
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

	// 6. Prepare Render Target to be used as Render Texture //
	TransitionResource(renderBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void SceneStage::SetScene(Scene* newScene)
{
	scene = newScene;
}

void SceneStage::SetSkydome(CD3DX12_GPU_DESCRIPTOR_HANDLE skydomeHandle)
{
	this->skydomeHandle = skydomeHandle;
}

void SceneStage::CreatePipeline()
{
	CD3DX12_DESCRIPTOR_RANGE1 cbvRanges[1]; // placeholder cause there is no guarantee they are next too each other...
	cbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 1); // Lighting buffer

	CD3DX12_DESCRIPTOR_RANGE1 srvRanges[1];
	srvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // Albedo, Normal, Metallic Roughess, Occlusion

	CD3DX12_DESCRIPTOR_RANGE1 skydomeRange[1];
	skydomeRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1); // Albedo, Normal, Metallic Roughess, Occlusion

	CD3DX12_ROOT_PARAMETER1 rootParameters[5];
	rootParameters[0].InitAsConstants(32, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // MVP & Model
	rootParameters[1].InitAsConstants(3, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // Scene info ( Camera... etc. ) 
	rootParameters[2].InitAsDescriptorTable(1, &cbvRanges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Lighting data
	rootParameters[3].InitAsDescriptorTable(1, &srvRanges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Textures
	rootParameters[4].InitAsDescriptorTable(1, &skydomeRange[0], D3D12_SHADER_VISIBILITY_PIXEL); // Skydome

	rootSignature = new DXRootSignature(rootParameters, _countof(rootParameters), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	pipeline = new DXPipeline("Source/Shaders/default.vertex.hlsl", "Source/Shaders/default.pixel.hlsl", rootSignature, true);
}