#include "Renderer.h"

#include "DXUtilities.h"
#include "DXAccess.h"
#include "DXDevice.h"
#include "DXCommands.h"
#include "DXPipeline.h"
#include "DXDescriptorHeap.h"
#include "Window.h"

#include <cassert>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3 Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3( 1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3( 1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3( 1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3( 1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static short g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

namespace RendererInternal
{
	DXDevice* device = nullptr;
	DXCommands* commands = nullptr;
}
using namespace RendererInternal;

Renderer::Renderer(std::wstring windowName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	device = new DXDevice();
	commands = new DXCommands();
	DSVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
	CBVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	window = new Window(windowName, startWindowWidth, startWindowHeight);

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	pipeline = new DXPipeline(L"triangle.vs.hlsl", L"triangle.ps.hlsl", rootParameters, 1);

	FOV = 60.0f; // Move to a camera class //

	LoadContent();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    

	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(window->GetHWND());
	ImGui_ImplDX12_Init(device->GetAddress(), Window::BackBufferCount, DXGI_FORMAT_R8G8B8A8_UNORM,
		CBVHeap->GetAddress(), CBVHeap->GetCPUHandleAt(0), CBVHeap->GetGPUHandleAt(0));
}

Renderer::~Renderer()
{
	commands->Flush();
	delete commands;
}

void Renderer::Render()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::ShowDemoWindow();

	ImGui::Render();

	// Placeholder until we've a proper update //
	float angle = static_cast<float>(frameCount * 4.15f);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
	model = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle)) * XMMatrixTranslation(cos(frameCount * 0.015) * 8.0f, sin(frameCount * 0.005) * 5.0f, 0.0f);

	const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
	view = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	float aspectRatio = (float)window->GetWindowWidth() / static_cast<float>(window->GetWindowHeight());
	projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(FOV), aspectRatio, 0.1f, 1000.0f);

	XMMATRIX mvp = XMMatrixMultiply(model, view);
	mvp = XMMatrixMultiply(mvp, projection);

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
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);

	// 4. Rasterizer Settings //
	commandList->RSSetViewports(1, &window->GetViewport());
	commandList->RSSetScissorRects(1, &window->GetScissorRect());

	// 5. Output-Merger Settings //
	commandList->OMSetRenderTargets(1, &renderTarget, FALSE, &depthBuffer);

	// 6. Bind Relevant Pipeline with the correct Root Signature //
	pipeline->Set();

	// 7. Set Constants and Views for the pipeline  //
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvp, 0);

	// 8. Draw call //
	commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

	ID3D12DescriptorHeap* heaps[] = { CBVHeap->GetAddress() };

	commandList->SetDescriptorHeaps(1, heaps);
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
	
	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	commands->ResetCommandList(backBufferIndex);

	// Upload vertex buffer //
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UploadBufferToResource(&vertexBuffer, &intermediateVertexBuffer, _countof(g_Vertices), 
		sizeof(VertexPosColor), g_Vertices);

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(g_Vertices);
	vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

	// Upload Index Buffer //
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UploadBufferToResource(&indexBuffer, &intermediateIndexBuffer, _countof(g_Indicies),
		sizeof(short), g_Indicies);

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	indexBufferView.SizeInBytes = sizeof(g_Indicies);

	// Create Depth-Stencil view heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&DSVHeap->Get())));

	commands->ExecuteCommandList(backBufferIndex);
	commands->WaitForFenceValue(backBufferIndex);

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