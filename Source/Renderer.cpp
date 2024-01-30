#include "Renderer.h"
#include "Logger.h"

// DirectX Components //
#include "DXAccess.h"
#include "DXDevice.h"
#include "DXCommands.h"
#include "DXUtilities.h"
#include "DXDescriptorHeap.h"
#include "DXRootSignature.h"
#include "DXPipeline.h"

// Renderer Components //
#include "Window.h"
#include "Model.h"
#include "Camera.h"

#include <cassert>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

namespace RendererInternal
{
	Window* window = nullptr;
	DXDevice* device = nullptr;

	DXCommands* directCommands = nullptr;
	DXCommands* copyCommands = nullptr;

	DXDescriptorHeap* CBVHeap = nullptr;
	DXDescriptorHeap* DSVHeap = nullptr;
	DXDescriptorHeap* RTVHeap = nullptr;
}
using namespace RendererInternal;

Renderer::Renderer(const std::wstring& applicationName)
{
	// Initialization for all vital components for rendering //
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	device = new DXDevice();

	CBVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	DSVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 5);
	RTVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3);

	directCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, Window::BackBufferCount);
	copyCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, 1);

	window = new Window(applicationName, width, height);
	camera = new Camera(width, height);

	InitializeImGui();

	// Pipeline // 
	CD3DX12_DESCRIPTOR_RANGE1 descriptorRanges[1];
	descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 1); // Lighting buffer

	CD3DX12_ROOT_PARAMETER1 rootParameters[3];
	rootParameters[0].InitAsConstants(32, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // MVP & Model
	rootParameters[1].InitAsConstants(3, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // Scene info ( Camera... etc. ) 
	rootParameters[2].InitAsDescriptorTable(1, &descriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Lighting data

	rootSignature = new DXRootSignature(rootParameters, 3, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	pipeline = new DXPipeline("Source/Shaders/default.vertex.hlsl", "Source/Shaders/default.pixel.hlsl", rootSignature);

	UpdateLightBuffer();
}

void Renderer::Update(float deltaTime)
{
	elaspedTime += deltaTime;

	camera->Update(deltaTime);

	ImGui::Begin("Lights");

	if(ImGui::Button("Add Light"))
	{
		if(lights.activePointLights < MAX_AMOUNT_OF_LIGHTS)
		{
			lights.activePointLights++;
			UpdateLightBuffer();
		}
		else
		{
			LOG(Log::MessageType::Debug, "You are already at the limit of lights")
		}
	}

	for(int i = 0; i < lights.activePointLights; i++)
	{
		ImGui::PushID(i);

		PointLight& pointLight = lights.pointLights[i];

		std::string name = "Light - " + std::to_string(i);
		ImGui::SeparatorText(name.c_str());

		if(ImGui::DragFloat3("Position", &pointLight.Position[0], 0.01f)) { UpdateLightBuffer(); }
		if(ImGui::ColorEdit3("Color", &pointLight.Color[0])) { UpdateLightBuffer(); }
		if(ImGui::DragFloat("Intensity", &pointLight.Intensity, 0.05f, 0.0f, 1000.0f)) { UpdateLightBuffer(); }

		ImGui::PopID();
	}

	ImGui::End();
}

void Renderer::Render()
{
	float aspectRatio = float(width) / float(height);

	// Grab all relevant objects for the draw call //
	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();
	ComPtr<ID3D12GraphicsCommandList2> commandList = directCommands->GetGraphicsCommandList();
	CD3DX12_CPU_DESCRIPTOR_HANDLE renderTarget = window->GetCurrentBackBufferRTV();
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv = DSVHeap->GetCPUHandleAt(0);

	// 1. Reset Command Allocator & List, afterwards prepare Render Target //
	directCommands->ResetCommandList(backBufferIndex);
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// 2. Clear Screen & Depth Buffer //
	float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	commandList->ClearRenderTargetView(renderTarget, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// NEW: Set Pipeline & Root //
	commandList->SetPipelineState(pipeline->GetAddress());
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	commandList->RSSetViewports(1, &window->GetViewport());
	commandList->RSSetScissorRects(1, &window->GetScissorRect());
	commandList->OMSetRenderTargets(1, &renderTarget, FALSE, &dsv);

	commandList->SetGraphicsRoot32BitConstants(1, 3, &camera->GetForwardVector(), 0);

	ID3D12DescriptorHeap* heaps[] = { CBVHeap->GetAddress() };
	commandList->SetDescriptorHeaps(1, heaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle = CBVHeap->GetGPUHandleAt(lightCBVIndex);
	commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);

	for (Model* model : models)
	{
		model->Draw(camera->GetViewProjectionMatrix());
	}

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

	// 3. Transition to a Present state, then execute the commands and present the next back buffer //
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	directCommands->ExecuteCommandList(backBufferIndex);
	window->Present();

	// 4. Before we go to the next cycle, we gotta make sure that back buffer is available for use //
	directCommands->WaitForFenceValue(window->GetCurrentBackBufferIndex());
}

void Renderer::Resize()
{
	directCommands->Flush();

	window->Resize();
	camera->ResizeProjectionMatrix(window->GetWindowWidth(), window->GetWindowHeight());
}

void Renderer::AddModel(const std::string& filePath)
{
	models.push_back(new Model(filePath));
}

void Renderer::InitializeImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window->GetHWND());

	const unsigned int cbvIndex = CBVHeap->GetNextAvailableIndex();
	ImGui_ImplDX12_Init(device->GetAddress(), Window::BackBufferCount, DXGI_FORMAT_R8G8B8A8_UNORM,
		CBVHeap->GetAddress(), CBVHeap->GetCPUHandleAt(cbvIndex), CBVHeap->GetGPUHandleAt(cbvIndex));
}

void Renderer::UpdateLightBuffer()
{
	if(lightCBVIndex < 0)
	{
		lightCBVIndex = CBVHeap->GetNextAvailableIndex();
	}

	UpdateInFlightCBV(lightBuffer.Get(), lightCBVIndex, 1, sizeof(LightData), &lights);
}

#pragma region DXAccess Implementations
ComPtr<ID3D12Device2> DXAccess::GetDevice()
{
	if(!device)
	{
		assert(false && "DXDevice hasn't been initialized yet, call will return nullptr");
	}

	return device->Get();
}

DXCommands* DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE type)
{
	if(!directCommands || !copyCommands)
	{
		assert(false && "Commands haven't been initialized yet, call will return nullptr");
	}

	switch(type)
	{
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return copyCommands;
		break;

	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return directCommands;
		break;
	}

	assert(false && "This command type hasn't been added yet!");
	return nullptr;
}

unsigned int DXAccess::GetCurrentBackBufferIndex()
{
	if(!window)
	{
		assert(false && "Window hasn't been initialized yet, can't retrieve back buffer index");
	}

	return window->GetCurrentBackBufferIndex();
}

DXDescriptorHeap* DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	switch(type)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		return CBVHeap;
		break;

	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		return DSVHeap;
		break;

	case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		return RTVHeap;
		break;
	}

	// Invalid type passed
	assert(false && "Descriptor type passed isn't a valid or created type!");
	return nullptr;
}
#pragma endregion