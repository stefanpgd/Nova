#include "Renderer.h"

#include "DXAccess.h"
#include "DXDevice.h"
#include "DXCommands.h"
#include "DXUtilities.h"
#include "DXDescriptorHeap.h"
#include "Window.h"

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <cassert>

namespace RendererInternal
{
	Window* window = nullptr;
	DXDevice* device = nullptr;
	DXCommands* commands = nullptr;

	DXDescriptorHeap* CSUHeap = nullptr;
	DXDescriptorHeap* DSVHeap = nullptr;
	DXDescriptorHeap* RTVHeap = nullptr;
}
using namespace RendererInternal;

Renderer::Renderer(const std::wstring& applicationName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	device = new DXDevice();

	CSUHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000);
	DSVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 5);
	RTVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3);

	commands = new DXCommands();
	window = new Window(applicationName, 1080, 720);
}

void Renderer::Render()
{
	// Grab all relevant objects for the draw call //
	unsigned int backBufferIndex = window->GetCurrentBackBufferIndex();
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();
	ComPtr<ID3D12GraphicsCommandList2> commandList = commands->GetCommandList();
	CD3DX12_CPU_DESCRIPTOR_HANDLE renderTarget = window->GetCurrentBackBufferRTV();

	// 1. Reset Command Allocator & List, afterwards prepare Render Target //
	commands->ResetCommandList(backBufferIndex);
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// 2. Clear Screen //
	float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	commandList->ClearRenderTargetView(renderTarget, clearColor, 0, nullptr);

	// 3. Transition to a Present state, then execute the commands and present the next back buffer //
	TransitionResource(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commands->ExecuteCommandList(backBufferIndex);
	window->Present();

	// 4. Before we go to the next cycle, we gotta make sure that back buffer is available for use //
	commands->WaitForFenceValue(window->GetCurrentBackBufferIndex());
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

DXCommands* DXAccess::GetCommands()
{
	if(!commands)
	{
		assert(false && "DXCommands hasn't been initialized yet, call will return nullptr");
	}

	return commands;
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
}
#pragma endregion