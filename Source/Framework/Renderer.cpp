#include "Framework/Renderer.h"
#include "Utilities/Logger.h"

// DirectX Components //
#include "Graphics/DXAccess.h"
#include "Graphics/DXDevice.h"
#include "Graphics/DXCommands.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXDescriptorHeap.h"
#include "Graphics/DXRootSignature.h"
#include "Graphics/DXPipeline.h"

// Renderer Components //
#include "Graphics/Window.h"
#include "Graphics/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Camera.h"
#include "Graphics/Texture.h"

// Render Stages //
#include "Graphics/RenderStages/SceneStage.h"
#include "Graphics/RenderStages/ScreenStage.h"
#include "Graphics/RenderStages/SkydomeStage.h"

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

// TODO: Move to window
Texture* screenBackBuffer[Window::BackBufferCount];

Renderer::Renderer(const std::wstring& applicationName)
{
	// Initialization for all vital components for rendering //
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	device = new DXDevice();

	CBVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	DSVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 5);
	RTVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 10);

	directCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, Window::BackBufferCount);
	copyCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, 1);

	window = new Window(applicationName, width, height);
	camera = new Camera(width, height);

	InitializeImGui();

	// Pipeline // 


	// TODO: Maybe do a countof or something so you know how many are inside of the rootParameters...


	UpdateLightBuffer();

	int bufferSize = window->GetWindowWidth() * window->GetWindowHeight() * 4;
	unsigned char* buffer = new unsigned char[bufferSize];

	for(int i = 0; i < 3; i++)
	{
		screenBackBuffer[i] = new Texture(buffer, window->GetWindowWidth(), window->GetWindowHeight());

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->GetCPUHandleAt(i + 3);

		ComPtr<ID3D12Resource> backBufferResource = screenBackBuffer[i]->GetResource();
		DXAccess::GetDevice()->CreateRenderTargetView(backBufferResource.Get(), nullptr, rtvHandle);
	}
	//for(int i = 0; i < bufferSize; i += 4)
	//{
	//	buffer[i] = 255;
	//	buffer[i + 1] = 255;
	//	buffer[i + 2] = 255;
	//	buffer[i + 3] = 255;
	//}

	delete[] buffer;

	sceneStage = new SceneStage(window, camera, models, &lights, lightCBVIndex);
	screenStage = new ScreenStage(window);
	skydomeStage = new SkydomeStage(window, camera);
}

// TODO: move light out of Update into Editor
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
	// 0. Grab all relevant objects for the draw call //
	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	ComPtr<ID3D12GraphicsCommandList2> commandList = directCommands->GetGraphicsCommandList();
	ID3D12DescriptorHeap* heaps[] = { CBVHeap->GetAddress() };

	CD3DX12_CPU_DESCRIPTOR_HANDLE renderTarget = window->GetCurrentBackBufferRTV();
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv = DSVHeap->GetCPUHandleAt(0);
	CD3DX12_GPU_DESCRIPTOR_HANDLE lightData = CBVHeap->GetGPUHandleAt(lightCBVIndex);

	// 1. Reset Command Allocator & List, afterwards prepare Render Target //
	directCommands->ResetCommandList(backBufferIndex);

	ComPtr<ID3D12Resource> testBuffer = screenBackBuffer[backBufferIndex]->GetResource();
	TransitionResource(testBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CD3DX12_CPU_DESCRIPTOR_HANDLE testTarget = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->GetCPUHandleAt(backBufferIndex + 3);

	// 3. Setup Information for the Pipeline //
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// TODO: Move to Bind And Clear??
	BindAndClearRenderTarget(window, &testTarget, &dsv);

	sceneStage->RecordStage(commandList);

	// 6. Apply draw data from UI / ImGui //
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

	// 7. Transition to a Present state, then execute the commands and present the next back buffer //
	TransitionResource(testBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// SCREEN ATTEMPT //
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	BindAndClearRenderTarget(window, &renderTarget, &dsv);

	skydomeStage->RecordStage(commandList);

	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Placeholder: Set SRV individual? // 
	CD3DX12_GPU_DESCRIPTOR_HANDLE screenTextureData = CBVHeap->GetGPUHandleAt(screenBackBuffer[backBufferIndex]->GetSRVIndex());

	screenStage->PlaceHolderFunction(screenTextureData);
	screenStage->RecordStage(commandList);

	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	directCommands->ExecuteCommandList(backBufferIndex);
	window->Present();

	// 8. Before we go to the next cycle, we gotta make sure that "next" back buffer is available for use //
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