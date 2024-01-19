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

Renderer::Renderer(const std::wstring& applicationName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	device = new DXDevice();

	CSUHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000);
	DSVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 5);
	RTVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3);

	directCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, Window::BackBufferCount);
	copyCommands = new DXCommands(D3D12_COMMAND_LIST_TYPE_DIRECT, 1);

	window = new Window(applicationName, width, height);

	CD3DX12_ROOT_PARAMETER1 rootParameters[1];
	rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	rootSignature = new DXRootSignature(rootParameters, 1, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	pipeline = new DXPipeline("Source/Shaders/default.vertex.hlsl", "Source/Shaders/default.pixel.hlsl", rootSignature);
	
	for (int i = 0; i < 5; i++)
	{
		meshes[i] = new Mesh();
	}
}

void Renderer::Render()
{
	// TODO: Move this stuff to an appropriate place
	float angle = float(frameCount) * (90.0 / 144.0f);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);

	matrix rot = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	float s = abs(cosf(float(frameCount) * 0.01));
	model = XMMatrixScaling(s, s, s);
	model = XMMatrixMultiply(model, rot);


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

	// NEW: Set Pipeline & Root //
	commandList->SetPipelineState(pipeline->GetAddress());
	commandList->SetGraphicsRootSignature(rootSignature->GetAddress());

	// NEW: Setup Input Assembler //
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

	// NEW: Rasterization Stage //
	commandList->RSSetViewports(1, &window->GetViewport());
	commandList->RSSetScissorRects(1, &window->GetScissorRect());

	// NEW: Setup Output Merger //
	commandList->OMSetRenderTargets(1, &renderTarget, FALSE, &dsv);

	// NEW: Bind constants (mvp) //
	commandList->SetGraphicsRoot32BitConstants(0, 16, &MVP, 0);

	// NEW: Draw Mesh //
	commandList->DrawIndexedInstanced(mesh->GetIndicesCount(), 1, 0, 0, 0);

	// 3. Transition to a Present state, then execute the commands and present the next back buffer //
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	directCommands->ExecuteCommandList(backBufferIndex);
	window->Present();

	// 4. Before we go to the next cycle, we gotta make sure that back buffer is available for use //
	directCommands->WaitForFenceValue(window->GetCurrentBackBufferIndex());

	frameCount++;
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