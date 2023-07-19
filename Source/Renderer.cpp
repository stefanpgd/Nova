#include "Renderer.h"

#include "DXUtilities.h"
#include "DXAccess.h"
#include "DXDevice.h"
#include "DXCommands.h"
#include "DXPipeline.h"
#include "DXDescriptorHeap.h"
#include "Window.h"
#include "Mesh.h"

#include <cassert>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "Model.h"
#include "Texture.h"

Model* model;
Texture* texture;

namespace RendererInternal
{
	Window* window = nullptr;
	DXDevice* device = nullptr;
	DXCommands* commands = nullptr;

	DXDescriptorHeap* CSUHeap = nullptr;
	DXDescriptorHeap* DSVHeap = nullptr;
}
using namespace RendererInternal;

Renderer::Renderer(std::wstring windowName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	device = new DXDevice();
	commands = new DXCommands();
	DSVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
	CSUHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	window = new Window(windowName, startWindowWidth, startWindowHeight);

	CD3DX12_DESCRIPTOR_RANGE1 descriptorTable[1];
	descriptorTable[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER1 rootParameters[3];
	rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsConstants(1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[2].InitAsDescriptorTable(1, &descriptorTable[0], D3D12_SHADER_VISIBILITY_PIXEL);
	pipeline = new DXPipeline(L"triangle.vs.hlsl", L"triangle.ps.hlsl", rootParameters, 3);

	FOV = 60.0f; // Move to a camera class //

	LoadContent();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    

	unsigned int imguiDescriptorIndex = CSUHeap->GetNextAvailableIndex();
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window->GetHWND());
	ImGui_ImplDX12_Init(device->GetAddress(), Window::BackBufferCount, DXGI_FORMAT_R8G8B8A8_UNORM,
		CSUHeap->GetAddress(), CSUHeap->GetCPUHandleAt(imguiDescriptorIndex), CSUHeap->GetGPUHandleAt(imguiDescriptorIndex));
}

Renderer::~Renderer()
{
	commands->Flush();
	delete commands;
}

float rotationSpeed = 1.0f;

void Renderer::Render()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Hello Window");
	ImGui::DragFloat3("Position", &model->meshes[0]->transform.Position[0], 0.02f);
	ImGui::DragFloat3("Rotation", &model->meshes[0]->transform.Rotation[0]);
	ImGui::DragFloat3("Scale", &model->meshes[0]->transform.Scale[0], 0.05f);
	ImGui::End();

	const glm::mat4 m = model->meshes[0]->transform.GetModelMatrix();
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	viewMatrix = glm::lookAtRH(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	projectionMatrix = glm::perspective(glm::radians(FOV), (float)window->GetWindowWidth() / (float)window->GetWindowHeight(), 0.01f, 100000.0f);

	glm::mat4 mvp = projectionMatrix * viewMatrix * m;

	ImGui::Render();

	// Grab all relevant objects for the draw call //
	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();
	ComPtr<ID3D12GraphicsCommandList2> commandList = commands->GetCommandList();
	CD3DX12_CPU_DESCRIPTOR_HANDLE renderTarget = window->GetCurrentBackBufferRTV();
	CD3DX12_CPU_DESCRIPTOR_HANDLE depthBuffer = DSVHeap->GetCPUHandleAt(0);

	// 1. Reset Command Allocator & List, afterwards prepare Render Target //
	commands->ResetCommandList(backBufferIndex);
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// 2. Clear Screen //
	float clearColor[4] = { 0.2f, 0.45f, 0.31f, 1.0f };
	commandList->ClearRenderTargetView(renderTarget, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(depthBuffer, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// 3. Input-Assembler Settings //
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 4. Rasterizer Settings //
	commandList->RSSetViewports(1, &window->GetViewport());
	commandList->RSSetScissorRects(1, &window->GetScissorRect());

	// 5. Output-Merger Settings //
	commandList->OMSetRenderTargets(1, &renderTarget, FALSE, &depthBuffer);

	// 6. Bind Relevant Pipeline with the correct Root Signature //
	pipeline->Set();

	float time = (float)frameCount / 60.0f;

	// 7. Set Heaps, Constants and Views for the pipeline  //
	ID3D12DescriptorHeap* heaps[] = { CSUHeap->GetAddress() };
	commandList->SetDescriptorHeaps(1, heaps);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(glm::mat4) / 4, &mvp[0], 0);
	commandList->SetGraphicsRoot32BitConstants(1, 1, &time, 0);

	// 8. Draw call //
	model->Draw();

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

	// 9. Transition to a Present state, then execute the commands and present the next back buffer //
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commands->ExecuteCommandList(backBufferIndex);
	window->Present();

	// 10. Before we start with the next draw call, ensure that the next frame isn't still in-flight //
	commands->WaitForFenceValue(window->GetCurrentBackBufferIndex());

	frameCount++;
}

void Renderer::Resize()
{
	commands->Flush();
	window->Resize();
	ResizeDepthBuffer();
}

void Renderer::LoadContent()
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();

	//model = new Model("Assets/Models/CylinderEngine/2CylinderEngine.gltf");
	//model = new Model("Assets/Models/Avocado/Avocado.gltf");
	//model = new Model("Assets/Models/SciFiHelmet/SciFiHelmet.gltf");
	model = new Model("Assets/Models/Car/scene.gltf");

	//texture = new Texture("Assets/Models/SciFiHelmet/SciFiHelmet_BaseColor.png");

	// Create Depth-Stencil view heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&DSVHeap->Get())));

	ResizeDepthBuffer();
}

void Renderer::ResizeDepthBuffer()
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	commands->Flush();
	depthBuffer.Reset();

	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0f, 0 };

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);;
	CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, window->GetWindowWidth(), window->GetWindowHeight(),
		1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &optimizedClearValue, IID_PPV_ARGS(&depthBuffer)));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	dsv.Format = DXGI_FORMAT_D32_FLOAT;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice = 0;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthBuffer.Get(), &dsv,
		DSVHeap->GetCPUHandleAt(0));
}

ComPtr<ID3D12Device2> DXAccess::GetDevice()
{
	if (device)
	{
		return device->Get();
	}

	assert(false && "DXDevice hasn't been initialized");
	return nullptr;
}

DXCommands* DXAccess::GetCommands()
{
	if (commands)
	{
		return commands;
	}

	assert(false && "DXCommands hasn't been initialized");
	return nullptr;
}

unsigned int DXAccess::GetCurrentBackBufferIndex()
{
	return window->GetCurrentBackBufferIndex();
}

DXDescriptorHeap* DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	switch (type)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		return CSUHeap;
		break;

	case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		return DSVHeap;
		break;
	}

	printf("Warning: type is not a valid Descriptor Heap type!");
	return nullptr;
}