#include "Framework/Renderer.h"
#include "Framework/Scene.h"

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
#include "Graphics/DepthBuffer.h"

// Render Stages //
#include "Graphics/RenderStages/ShadowStage.h"
#include "Graphics/RenderStages/SceneStage.h"
#include "Graphics/RenderStages/ScreenStage.h"
#include "Graphics/RenderStages/SkydomeStage.h"
#include "Graphics/RenderStages/HDRIConvolutionStage.h"

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

	Texture* defaultTexture = nullptr;
}
using namespace RendererInternal;

Renderer::Renderer(const std::wstring& applicationName, Scene* scene, unsigned int windowWidth, 
	unsigned int windowHeight) : scene(scene)
{
	// Initialization for all vital components for rendering //
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	device = new DXDevice();
	CBVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 5000, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	DSVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 10);
	RTVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 15);

	directCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, Window::BackBufferCount);
	copyCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, 1);

	window = new Window(applicationName, windowWidth, windowHeight);
	defaultTexture = new Texture("Assets/Textures/error.jpg");

	InitializeImGui();

	shadowStage = new ShadowStage(window, scene);
	sceneStage = new SceneStage(window, scene, shadowStage);
	screenStage = new ScreenStage(window);
	skydomeStage = new SkydomeStage(window, scene);
	convolutionStage = new HDRIConvolutionStage(window);

	sceneStage->SetSkydome(skydomeStage->GetSkydomeHandle());
	convolutionStage->BindHDRI(skydomeStage->GetHDRI());

	// TODO: Move to scene
	//this->scene->AddModel("Assets/Models/GroundPlane\\plane.gltf");
}

void Renderer::Update(float deltaTime) 
{ 
	shadowStage->Update(deltaTime);

	skydomeStage->SetScene(scene);
	// TODO: Maybe record render times in here, have a proper MS count etc.
}

void Renderer::Render()
{
	// 0. Grab all relevant objects to perform all stages //
	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	ComPtr<ID3D12GraphicsCommandList2> commandList = directCommands->GetGraphicsCommandList();
	ID3D12DescriptorHeap* heaps[] = { CBVHeap->GetAddress() };

	// 1. Reset & prepare command allocator //
	directCommands->ResetCommandList(backBufferIndex);

	// 2. Bind general resources & Set pipeline parameters //
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 3. Record Render Stages //
	convolutionStage->RecordStage(commandList);
	shadowStage->RecordStage(commandList);
	sceneStage->RecordStage(commandList);
	skydomeStage->RecordStage(commandList);
	screenStage->RecordStage(commandList);

	// 4. Execute List, Present and wait for the next frame to be ready //
	directCommands->ExecuteCommandList(backBufferIndex);
	window->Present();
	directCommands->WaitForFenceValue(window->GetCurrentBackBufferIndex());
}

void Renderer::SetScene(Scene* newScene)
{
	scene = newScene;
}

void Renderer::Resize()
{
	directCommands->Flush();
	window->Resize();
	scene->GetCamera().ResizeProjectionMatrix(window->GetWindowWidth(), window->GetWindowHeight());
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

Texture* DXAccess::GetDefaultTexture()
{
	// Incase an texture isn't present, the 'default' texture gets loaded in
	// Similar to Valve's ERROR 3D model
	return defaultTexture;
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

Window* DXAccess::GetWindow()
{
	return window;
}
#pragma endregion