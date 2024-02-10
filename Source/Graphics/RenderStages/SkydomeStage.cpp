#include "Graphics/RenderStages/SkydomeStage.h"

#include "Graphics/Camera.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXPipeline.h"
#include "Graphics/DXRootSignature.h"
#include "Graphics/DXDescriptorHeap.h"

#include <d3dx12.h>

SkydomeStage::SkydomeStage(Window* window, Camera* camera) : RenderStage(window), camera(camera)
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
	CD3DX12_GPU_DESCRIPTOR_HANDLE skydomeData = CBVHeap->GetGPUHandleAt(skydomeTexture->GetSRVIndex());

	// 1. Bind pipeline & root //
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	commandList->SetPipelineState(pipeline->GetAddress());

	// 2. Bind skydome texture & Forward Vector //
	commandList->SetGraphicsRootDescriptorTable(0, skydomeData);
	commandList->SetGraphicsRoot32BitConstants(1, 3, &camera->GetForwardVector(), 0);

	// 3. Render Skydome (mesh) //
	commandList->IASetVertexBuffers(0, 1, &skydomeMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&skydomeMesh->GetIndexBufferView());
	commandList->DrawIndexedInstanced(skydomeMesh->GetIndicesCount(), 1, 0, 0, 0);
}

void SkydomeStage::CreatePipeline()
{
	CD3DX12_DESCRIPTOR_RANGE1 skydomeRange[1];
	skydomeRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // Skydome Texture

	CD3DX12_ROOT_PARAMETER1 skydomeRootParameters[2];
	skydomeRootParameters[0].InitAsDescriptorTable(1, &skydomeRange[0], D3D12_SHADER_VISIBILITY_PIXEL);
	skydomeRootParameters[1].InitAsConstants(3, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	rootSignature = new DXRootSignature(skydomeRootParameters, _countof(skydomeRootParameters), 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	pipeline = new DXPipeline("Source/Shaders/skydome.vertex.hlsl", "Source/Shaders/skydome.pixel.hlsl", rootSignature);

}