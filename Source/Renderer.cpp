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

ComPtr<ID3D12Resource> vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

ComPtr<ID3D12Resource> indexBuffer;
D3D12_INDEX_BUFFER_VIEW indexBufferView;

ComPtr<ID3D12Resource> depthBuffer;

ComPtr<ID3D12RootSignature> rootSignature;
ComPtr<ID3D12PipelineState> pipeline;

matrix model;
matrix view;
matrix projection;
float FOV = 60.0f;

struct Vertex
{
	float3 Position;
	float3 Color;
};

static Vertex cubeBuffer[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },  // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },  // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },  // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }   // 7
};

static unsigned int cubeIndices[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 6, 5, 4, 7, 6,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

// Process:
// We want to upload our buffer from the CPU to the GPU
// To do that we've to go from: CPU -> System Memory (RAM) -> GPU
// Because of that we want to allocate TWO committed resources
// One that rests in the `DEFAULT_HEAP`, which is GPU memory
// And a "temporary" one that rests in the `UPLOAD_HEAP`, which is the system memory
// The temporary or, intermediate resource can be destroyed after uploading
// Heaps aren't only GPU... there are multiple types in different places
// Default Heap = (GPU) VRAM
// Upload Heap = system RAM
void UpdateBufferResource(ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource,
	unsigned int numberOfElements, unsigned int elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	if(!bufferData)
	{
		assert(false && "Buffer data is NOT valid!");
		return;
	}

	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	unsigned int bufferSize = numberOfElements * elementSize;

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	CD3DX12_RESOURCE_DESC bufferDescription = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);

	// Creating the resource on the GPU. Through CommitedResource we don't have to allocate the heap for it
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferDescription, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(destinationResource)));

	// Create a resource to the upload heap with the same parameters 
	ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferDescription, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(intermediateResource)));
	// Any resource within the upload heap MUST be GENERIC_READ

	// Describe the data that needs to be uploaded
	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = bufferData;
	subresourceData.RowPitch = bufferSize;
	subresourceData.SlicePitch = subresourceData.RowPitch;

	// TODO: Solve this mess...
	// SUPER TEMP //
	DXAccess::GetCommands()->ResetCommandList(DXAccess::GetCurrentBackBufferIndex());

	UpdateSubresources(DXAccess::GetCommands()->GetCommandList().Get(),
		*destinationResource, *intermediateResource, 0, 0, 1, &subresourceData);

	// SUPER TEMP //
	DXAccess::GetCommands()->ExecuteCommandList(DXAccess::GetCurrentBackBufferIndex());
	DXAccess::GetCommands()->Signal();
	DXAccess::GetCommands()->WaitForFenceValue(DXAccess::GetCurrentBackBufferIndex());
}

Renderer::Renderer(const std::wstring& applicationName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	device = new DXDevice();

	CSUHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1000);
	DSVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 5);
	RTVHeap = new DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3);

	commands = new DXCommands();
	window = new Window(applicationName, 1080, 720);

	// TODO: Remove executing of commandlist from inside the Upload function to outside 
	// TEMP //
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(&vertexBuffer, &intermediateVertexBuffer, _countof(cubeBuffer), 
		sizeof(Vertex), cubeBuffer, D3D12_RESOURCE_FLAG_NONE);

	// Create vertex view from the resources we just initialized
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = sizeof(cubeBuffer);
	vertexBufferView.StrideInBytes = sizeof(Vertex);

	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(&indexBuffer, &intermediateIndexBuffer, _countof(cubeIndices),
		sizeof(unsigned int), cubeIndices, D3D12_RESOURCE_FLAG_NONE);

	// Create vertex view from the resources we just initialized
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(cubeIndices);
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
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
	return nullptr;
}
#pragma endregion