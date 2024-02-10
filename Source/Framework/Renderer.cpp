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
#include "Graphics/RenderStages/ScreenStage.h"

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

// TODO: Make own class?
Texture* skydome;
Texture* screenBackBuffer[Window::BackBufferCount];
Model* skydomeModel;
Mesh* skyMesh;

DXRootSignature* skyRoot;
DXPipeline* skyPipeline;

ScreenStage* screenStage;

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
	CD3DX12_DESCRIPTOR_RANGE1 cbvRanges[1]; // placeholder cause there is no guarantee they are next too each other...
	cbvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 1); // Lighting buffer

	CD3DX12_DESCRIPTOR_RANGE1 srvRanges[1];
	srvRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // Render Target as Texture

	CD3DX12_DESCRIPTOR_RANGE1 skydomeRange[1];
	skydomeRange[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	CD3DX12_ROOT_PARAMETER1 rootParameters[5];
	rootParameters[0].InitAsConstants(32, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX); // MVP & Model
	rootParameters[1].InitAsConstants(3, 1, 0, D3D12_SHADER_VISIBILITY_VERTEX); // Scene info ( Camera... etc. ) 
	rootParameters[2].InitAsDescriptorTable(1, &cbvRanges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Lighting data
	rootParameters[3].InitAsDescriptorTable(1, &srvRanges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Textures
	rootParameters[4].InitAsDescriptorTable(1, &skydomeRange[0], D3D12_SHADER_VISIBILITY_PIXEL); // Testing 

	// TODO: Maybe do a countof or something so you know how many are inside of the rootParameters...
	rootSignature = new DXRootSignature(rootParameters, _countof(rootParameters), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	pipeline = new DXPipeline("Source/Shaders/default.vertex.hlsl", "Source/Shaders/default.pixel.hlsl", rootSignature);

	UpdateLightBuffer();

	skydome = new Texture("Assets/HDRI/testDome.hdr");

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

	skydomeModel = new Model("Assets/Models/Skydome/skydome.gltf");
	skyMesh = skydomeModel->GetMesh(0);


	CD3DX12_ROOT_PARAMETER1 skyRootParams[2];
	skyRootParams[0].InitAsDescriptorTable(1, &srvRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);
	skyRootParams[1].InitAsConstants(3, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	skyRoot = new DXRootSignature(skyRootParams, _countof(skyRootParams), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	skyPipeline = new DXPipeline("Source/Shaders/skydome.vertex.hlsl", "Source/Shaders/skydome.pixel.hlsl", skyRoot);

	screenStage = new ScreenStage(window);
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

	// 2. Clear Screen & Depth Buffer //
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList->ClearRenderTargetView(testTarget, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 3. Setup Information for the Pipeline //
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());
	commandList->SetPipelineState(pipeline->GetAddress());

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->RSSetViewports(1, &window->GetViewport());
	commandList->RSSetScissorRects(1, &window->GetScissorRect());
	commandList->OMSetRenderTargets(1, &testTarget, FALSE, &dsv);

	// 4. Set the root arguments // 
	commandList->SetGraphicsRoot32BitConstants(1, 3, &camera->Position, 0);
	commandList->SetGraphicsRootDescriptorTable(2, lightData);

	// Placeholder: Set SRV individual? // 
	CD3DX12_GPU_DESCRIPTOR_HANDLE skydomeData = CBVHeap->GetGPUHandleAt(skydome->GetSRVIndex());
	commandList->SetGraphicsRootDescriptorTable(4, skydomeData);

	// 5. Draw Calls & Bind MVPs ( happens in Model.cpp ) // 
	for(Model* model : models)
	{
		model->Draw(camera->GetViewProjectionMatrix());
	}

	// 6. Apply draw data from UI / ImGui //
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

	// 7. Transition to a Present state, then execute the commands and present the next back buffer //
	TransitionResource(testBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// SCREEN ATTEMPT //
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	commandList->ClearRenderTargetView(renderTarget, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	commandList->OMSetRenderTargets(1, &renderTarget, FALSE, &dsv);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->RSSetViewports(1, &window->GetViewport());
	commandList->RSSetScissorRects(1, &window->GetScissorRect());

	commandList->SetGraphicsRootSignature(skyRoot->GetAddress());
	commandList->SetPipelineState(skyPipeline->GetAddress());

	// Placeholder: Set SRV individual? // 
	commandList->SetGraphicsRootDescriptorTable(0, skydomeData);
	commandList->SetGraphicsRoot32BitConstants(1, 3, &camera->GetForwardVector(), 0);

	commandList->IASetVertexBuffers(0, 1, &skyMesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&skyMesh->GetIndexBufferView());
	commandList->DrawIndexedInstanced(skyMesh->GetIndicesCount(), 1, 0, 0, 0);

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