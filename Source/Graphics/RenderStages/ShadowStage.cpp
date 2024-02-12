#include "Graphics/RenderStages/ShadowStage.h"

#include "Framework/Scene.h"

#include "Graphics/DepthBuffer.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXRootSignature.h"
#include "Graphics/DXPipeline.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include <d3dx12.h>

ShadowStage::ShadowStage(Window* window, Scene* scene) : RenderStage(window), scene(scene)
{
	depthBuffer = new DepthBuffer(depthBufferWidth, depthBufferHeight);

	scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, 
		static_cast<float>(depthBufferWidth), static_cast<float>(depthBufferHeight));


	glm::mat4 view = glm::lookAt(glm::vec3(5.0f, 0.0f, 0.0), glm::vec3(5.0f, 0.0f, 0.0) - glm::vec3(-1.0f, 0.0f, 0.0), glm::vec3(0.0f, 1.0f, 0.0));
	glm::mat4 projection = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, shadowNear, shadowFar);
	lightMatrix = projection * view;

	CreatePipeline();
}

void ShadowStage::RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthView = depthBuffer->GetDSV();

	// 1. Clear Light DepthBuffer //
	commandList->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 2. Bind pipeline & root // 
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	commandList->SetPipelineState(pipeline->GetAddress());

	// Bind viewport & rect, followed by the depth buffer //
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->OMSetRenderTargets(0, nullptr, FALSE, &depthView);

	// Render scene //
	commandList->SetGraphicsRoot32BitConstants(0, 16, &lightMatrix, 0);

	for(Model* model : scene->GetModels())
	{
		for(Mesh* mesh : model->GetMeshes())
		{
			commandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
			commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

			commandList->DrawIndexedInstanced(mesh->GetIndicesCount(), 1, 0, 0, 0);
		}
	}
}

void ShadowStage::CreatePipeline()
{
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // Light Matrix

	rootSignature = new DXRootSignature(rootParameters, _countof(rootParameters), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// TODO: replace constructor with the newer one once available
	// TODO: Test is alpha blending affects shadows, it will so test how it can be more accurate
	pipeline = new DXPipeline("Source/Shaders/shadow.vertex.hlsl", "Source/Shaders/default.pixel.hlsl", rootSignature, false, false);
}