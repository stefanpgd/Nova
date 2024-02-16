#include "Graphics/RenderStages/ShadowStage.h"

#include "Framework/Scene.h"

#include "Graphics/DepthBuffer.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXRootSignature.h"
#include "Graphics/DXPipeline.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include <d3dx12.h>

#include <imgui.h>

ShadowStage::ShadowStage(Window* window, Scene* scene) : RenderStage(window), scene(scene)
{
	depthBuffer = new DepthBuffer(depthBufferWidth, depthBufferHeight);

	scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, 
		static_cast<float>(depthBufferWidth), static_cast<float>(depthBufferHeight));

	lightPosition = glm::vec3(-11.000, 11.000, 0.0);
	lightDirection = glm::normalize(glm::vec3(0.675, -0.738, 0.0f));

	CreatePipeline();
}

void ShadowStage::Update(float deltaTime)
{
	// Temp until we move it fully to editor //
	CD3DX12_GPU_DESCRIPTOR_HANDLE depthHandle = depthBuffer->GetSRV();

	ImGui::Begin("Depth Buffer");
	ImGui::Image((ImTextureID)depthHandle.ptr, ImVec2(256, 256));
	ImGui::End();

	ImGui::Begin("Depth Stage Settings");
	ImGui::DragFloat("Near", &shadowNear);
	ImGui::DragFloat("Far", &shadowFar);
	ImGui::DragFloat("Ortho Size", &orthoSize);

	ImGui::Separator();
	ImGui::DragFloat3("Light Position", &lightPosition[0]);
	ImGui::DragFloat3("Light Direction", &lightDirection[0], 0.01f);
	ImGui::End();

	lightDirection = glm::normalize(lightDirection);

	glm::mat4 view = glm::lookAt(lightPosition, lightPosition + lightDirection, glm::vec3(0.0f, 1.0f, 0.0));
	glm::mat4 projection = glm::perspective(glm::radians(55.0f), 1.0f, shadowNear, shadowFar);
	lightMatrix = projection * view;
}

void ShadowStage::RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	ComPtr<ID3D12Resource> depthResource = depthBuffer->GetResource();
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthView = depthBuffer->GetDSV();

	TransitionResource(depthResource.Get(), 
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

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
	for(Model* model : scene->GetModels())
	{
		commandList->SetGraphicsRoot32BitConstants(0, 16, &lightMatrix, 0);
		commandList->SetGraphicsRoot32BitConstants(0, 16, &model->Transform.GetModelMatrix(), 16);

		for(Mesh* mesh : model->GetMeshes())
		{
			commandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
			commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

			commandList->DrawIndexedInstanced(mesh->GetIndicesCount(), 1, 0, 0, 0);
		}
	}

	TransitionResource(depthResource.Get(),
	D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

DepthBuffer* ShadowStage::GetDepthBuffer()
{
	return depthBuffer;
}

const glm::mat4& ShadowStage::GetLightMatrix()
{
	return lightMatrix;
}

void ShadowStage::CreatePipeline()
{
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(32, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // Light & Model Matrix

	rootSignature = new DXRootSignature(rootParameters, _countof(rootParameters), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// TODO: replace constructor with the newer one once available
	// TODO: Test is alpha blending affects shadows, it will so test how it can be more accurate
	pipeline = new DXPipeline("Source/Shaders/shadow.vertex.hlsl", "Source/Shaders/default.pixel.hlsl", rootSignature, false, false);
}