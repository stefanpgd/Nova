#include "Graphics/RenderStages/ScreenStage.h"

#include "Graphics/DXAccess.h"
#include "Graphics/DXPipeline.h"
#include "Graphics/DXRootSignature.h"
#include "Graphics/DXDescriptorHeap.h"
#include "Graphics/Mesh.h"

ScreenStage::ScreenStage(Window* window) : RenderStage(window)
{
	CreatePipeline();
	CreateScreenMesh();
}

void ScreenStage::PlaceHolderFunction(CD3DX12_GPU_DESCRIPTOR_HANDLE handle)
{
	this->handle = handle;
}

void ScreenStage::RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	// 0. Grab all relevant variables //
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->GetCPUHandleAt(0);

	// 1. Bind pipeline & root // 
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	commandList->SetPipelineState(pipeline->GetAddress());

	// 2. Clear depth buffer to be (re-)used //
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 3. Use Render Target as Texture //
	// Temporarly get handle via placeholderfunction... 
	commandList->SetGraphicsRootDescriptorTable(0, handle);

	// 4. Bind & Render Screen Pass //
	commandList->IASetVertexBuffers(0, 1, &screenMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&screenMesh->GetIndexBufferView());
	commandList->DrawIndexedInstanced(screenMesh->GetIndicesCount(), 1, 0, 0, 0);
}

void ScreenStage::CreatePipeline()
{
	CD3DX12_DESCRIPTOR_RANGE1 screenRange[1];
	screenRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // Render Target as Texture

	CD3DX12_ROOT_PARAMETER1 screenRootParameters[1];
	screenRootParameters[0].InitAsDescriptorTable(1, &screenRange[0], D3D12_SHADER_VISIBILITY_PIXEL);

	rootSignature = new DXRootSignature(screenRootParameters, _countof(screenRootParameters),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	pipeline = new DXPipeline("Source/Shaders/screen.vertex.hlsl", "Source/Shaders/screen.pixel.hlsl", rootSignature);
}

void ScreenStage::CreateScreenMesh()
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