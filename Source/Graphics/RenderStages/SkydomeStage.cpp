#include "Graphics/RenderStages/SkydomeStage.h"

#include "Framework/Scene.h"

#include "Graphics/Camera.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXPipeline.h"
#include "Graphics/DXRootSignature.h"
#include "Graphics/DXDescriptorHeap.h"

#include <d3dx12.h>

SkydomeStage::SkydomeStage(Window* window, Scene* scene) : RenderStage(window), scene(scene)
{
	CreatePipeline();

	Model* skydome = new Model("Assets/Models/Skydome/skydome.gltf");
	skydomeMesh = skydome->GetMesh(0);

	skydomeTexture = new Texture("Assets/HDRI/testDome.hdr");
}

void SkydomeStage::RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	// 0. Get all relevant objects //
	DXDescriptorHeap* CBVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	DXDescriptorHeap* DSVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	ComPtr<ID3D12Resource> screenBuffer = window->GetCurrentScreenBuffer();
	CD3DX12_CPU_DESCRIPTOR_HANDLE screenRTV = window->GetCurrentScreenRTV();
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthView = DSVHeap->GetCPUHandleAt(0);
	CD3DX12_GPU_DESCRIPTOR_HANDLE skydomeData = CBVHeap->GetGPUHandleAt(skydomeTexture->GetSRVIndex());
	Camera& camera = scene->GetCamera();

	glm::mat4 view = glm::lookAt(glm::vec3(0.0f), camera.GetForwardVector(), camera.GetUpwardVector());
	glm::mat4 projection = camera.GetProjectionMatrix();
	skydomeMatrix = projection * view;

	// 1. Prepare the screen buffer to be used as Render Target //
	TransitionResource(screenBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	BindAndClearRenderTarget(window, &screenRTV, &depthView);

	// 2. Bind pipeline & root //
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	commandList->SetPipelineState(pipeline->GetAddress());

	// 3. Bind root arguments //
	commandList->SetGraphicsRootDescriptorTable(0, skydomeData);
	commandList->SetGraphicsRoot32BitConstants(1, 3, &camera.GetForwardVector(), 0);
	commandList->SetGraphicsRoot32BitConstants(2, 16, &skydomeMatrix, 0);

	// 4. Render Skydome (mesh) //
	commandList->IASetVertexBuffers(0, 1, &skydomeMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&skydomeMesh->GetIndexBufferView());
	commandList->DrawIndexedInstanced(skydomeMesh->GetIndicesCount(), 1, 0, 0, 0);
}

void SkydomeStage::SetScene(Scene* newScene)
{
	scene = newScene;
}

void SkydomeStage::CreatePipeline()
{
	CD3DX12_DESCRIPTOR_RANGE1 skydomeRange[1];
	skydomeRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // Skydome Texture

	CD3DX12_ROOT_PARAMETER1 skydomeRootParameters[3];
	skydomeRootParameters[0].InitAsDescriptorTable(1, &skydomeRange[0], D3D12_SHADER_VISIBILITY_PIXEL); // Skydome Texture
	skydomeRootParameters[1].InitAsConstants(3, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // Camera Forward
	skydomeRootParameters[2].InitAsConstants(16, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // MVP

	rootSignature = new DXRootSignature(skydomeRootParameters, _countof(skydomeRootParameters), 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	pipeline = new DXPipeline("Source/Shaders/skydome.vertex.hlsl", "Source/Shaders/skydome.pixel.hlsl", rootSignature);
}