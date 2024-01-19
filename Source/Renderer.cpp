#include "Renderer.h"

#include "DXAccess.h"
#include "DXDevice.h"
#include "DXCommands.h"
#include "DXUtilities.h"
#include "DXDescriptorHeap.h"
#include "DXRootSignature.h"
#include "DXPipeline.h"
#include "Window.h"
#include "Mesh.h"

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <d3dcompiler.h>
#include <cassert>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx12.h>

// TODO: Move depth stencil creation to the Window
// TODO: Add depth stencil resize function and call it during actual resize...

namespace RendererInternal
{
	Window* window = nullptr;
	DXDevice* device = nullptr;

	DXCommands* directCommands = nullptr;
	DXCommands* copyCommands = nullptr;

	DXDescriptorHeap* CSUHeap = nullptr;
	DXDescriptorHeap* DSVHeap = nullptr;
	DXDescriptorHeap* RTVHeap = nullptr;
}
using namespace RendererInternal;

matrix model;
matrix view;
matrix projection;
float FOV = 45.0f;
std::vector<Mesh*> meshes;

Mesh* mesh;

Renderer::Renderer(const std::wstring& applicationName)
{
	// Initialization for all vital components for rendering //
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	device = new DXDevice();

	CSUHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	DSVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 5);
	RTVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3);

	directCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, Window::BackBufferCount);
	copyCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, 1);

	window = new Window(applicationName, width, height);

	InitializeImGui();

	// Pipeline & Test Meshes // 
	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	rootSignature = new DXRootSignature(rootParameters, 1, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	pipeline = new DXPipeline("Source/Shaders/default.vertex.hlsl", "Source/Shaders/default.pixel.hlsl", rootSignature);
	
	for (int i = 0; i < 5; i++)
	{
		meshes.push_back(new Mesh());
	}
}

void Renderer::Render()
{
	// TODO: add a start frame function //
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// move to update? or somewhere else in engine
	ImGui::ShowDemoWindow();

	ImGui::Render();

	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	view = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	float aspectRatio = float(width) / float(height);
	projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(FOV), aspectRatio, 0.01f, 1000.0f);

	// NEW: Create MVP //
	matrix MVP = XMMatrixMultiply(model, view);
	MVP = XMMatrixMultiply(MVP, projection);

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

	float x = -5;
	for (Mesh* mesh : meshes)
	{
		float y = cosf(float(frameCount) / 1444.0 + (x * 0.5)) * 1.25f;
		model = XMMatrixTranslation(x, y, 0);
		const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);

		matrix rot = XMMatrixRotationY(XMConvertToRadians(float(frameCount) / 54.0 * x));
		model = XMMatrixMultiply(rot, model);

		float s = 1.0 - (abs(x) * 0.15f);
		matrix scale = XMMatrixScalingFromVector(XMVectorSet(s, s, s, 1.0f));
		model = XMMatrixMultiply(scale, model);

		matrix MVP = XMMatrixMultiply(model, view);
		MVP = XMMatrixMultiply(MVP, projection);

		commandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
		commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

		// Mvp needs to be set in Model, for now in here
		commandList->SetGraphicsRoot32BitConstants(0, 16, &MVP, 0);

		commandList->DrawIndexedInstanced(mesh->GetIndicesCount(), 1, 0, 0, 0);
		x += 2.5;
	}


	ID3D12DescriptorHeap* heaps[] = { CSUHeap->GetAddress() };

	commandList->SetDescriptorHeaps(1, heaps);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

	// 3. Transition to a Present state, then execute the commands and present the next back buffer //
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	directCommands->ExecuteCommandList(backBufferIndex);
	window->Present();

	// 4. Before we go to the next cycle, we gotta make sure that back buffer is available for use //
	directCommands->WaitForFenceValue(window->GetCurrentBackBufferIndex());

	frameCount++;
}

void Renderer::InitializeImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window->GetHWND());

	const unsigned int cbvIndex = CSUHeap->GetNextAvailableIndex();
	ImGui_ImplDX12_Init(device->GetAddress(), Window::BackBufferCount, DXGI_FORMAT_R8G8B8A8_UNORM,
		CSUHeap->GetAddress(), CSUHeap->GetCPUHandleAt(cbvIndex), CSUHeap->GetGPUHandleAt(cbvIndex));
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
		return CSUHeap;
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