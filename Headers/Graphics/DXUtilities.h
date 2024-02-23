#pragma once

#if defined(max)
#undef max
#endif

#include <exception>
#include <cassert>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Utilities/Logger.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXCommands.h"
#include "Graphics/DXDescriptorHeap.h"
#include "Window.h"

inline void ThrowIfFailed(HRESULT hr)
{
	if(FAILED(hr))
	{
		throw std::exception();
	}
}

inline void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	ComPtr<ID3D12GraphicsCommandList2> commandList = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetGraphicsCommandList();
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
	commandList->ResourceBarrier(1, &barrier);
}

// UpdateBufferResource Process:
// We want to upload our buffer from the CPU to the GPU
// To do that we've to go from: CPU -> System Memory (RAM) -> GPU
// Because of that we want to allocate TWO committed resources
// One that rests in the `DEFAULT_HEAP`, which is GPU memory
// And a "temporary" one that rests in the `UPLOAD_HEAP`, which is the system memory
// The temporary or, intermediate resource can be destroyed after uploading
// Heaps aren't only GPU... there are multiple types in different places
// Default Heap = (GPU) VRAM
// Upload Heap = system RAM
inline void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList2> commandList, ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource,
	unsigned int numberOfElements, unsigned int elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	if(!bufferData)
	{
		LOG(Log::MessageType::Error, "Buffer data is NOT valid!");
		assert(false);
	}

	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	unsigned int bufferSize = numberOfElements * elementSize;

	CD3DX12_RESOURCE_DESC bufferDescription = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Creating the resource on the GPU. Through CommitedResource we don't have to allocate the heap for it
	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferDescription, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(destinationResource)));

	// Create a resource to the upload heap with the same parameters 
	// Any resource within the upload heap MUST be GENERIC_READ
	ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&bufferDescription, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(intermediateResource)));

	// Describe the data that needs to be uploaded
	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = bufferData;
	subresourceData.RowPitch = bufferSize;
	subresourceData.SlicePitch = subresourceData.RowPitch;

	UpdateSubresources(commandList.Get(), *destinationResource, *intermediateResource, 0, 0, 1, &subresourceData);
}

inline void UploadPixelShaderResource(ComPtr<ID3D12Resource>& destinationResource, ComPtr<ID3D12Resource>& intermediateResource, D3D12_RESOURCE_DESC& resourceDescription, 
	D3D12_SUBRESOURCE_DATA& subresource)
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	DXCommands* copyCommands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_COPY);
	ComPtr<ID3D12GraphicsCommandList2> commandList = copyCommands->GetGraphicsCommandList();

	D3D12_HEAP_PROPERTIES gpuHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_HEAP_PROPERTIES uploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// 1.a Allocate heap on the GPU to upload the texture  //
	ThrowIfFailed(device->CreateCommittedResource(&gpuHeap, D3D12_HEAP_FLAG_NONE,
		&resourceDescription, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&destinationResource)));

	// 1.b Allocate heap in RAM to upload the texture to //
	unsigned int size = GetRequiredIntermediateSize(destinationResource.Get(), 0, 1);
	D3D12_RESOURCE_DESC bufferDescription = CD3DX12_RESOURCE_DESC::Buffer(size);

	ThrowIfFailed(device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE,
		&bufferDescription, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource)));

	// 2. Prepare barriers & record commands //
	CD3DX12_RESOURCE_BARRIER copyBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destinationResource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	CD3DX12_RESOURCE_BARRIER pixelBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destinationResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
	copyCommands->Flush();
	copyCommands->ResetCommandList();

	commandList->ResourceBarrier(1, &copyBarrier);
	UpdateSubresources(commandList.Get(), destinationResource.Get(), intermediateResource.Get(), 0, 0, 1, &subresource);
	commandList->ResourceBarrier(1, &pixelBarrier);

	// 3. Execute upload and wait until it's finished // 
	copyCommands->ExecuteCommandList();
	copyCommands->Signal();
	copyCommands->WaitForFenceValue();
}

// Ensures that the direct queue is paused so that a resource and its data can be updated 
inline void UpdateInFlightCBV(ComPtr<ID3D12Resource>& destinationResource, unsigned int CBVIndex, unsigned int numberOfElements, 
	unsigned int elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE)
{
	if(!bufferData)
	{
		LOG(Log::MessageType::Error, "Buffer data is NOT valid!");
		assert(false);
	}

	unsigned int bufferSize = numberOfElements * elementSize;
	if(bufferSize % 256 != 0)
	{
		LOG(Log::MessageType::Error, "Buffer data is NOT 256-byte aligned!");
		assert(false);
	}

	// Using direct to ensure the lights aren't in-flight //
	DXCommands* directCommands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT);
	directCommands->Flush();

	ComPtr<ID3D12GraphicsCommandList2> commandList = directCommands->GetGraphicsCommandList();
	directCommands->ResetCommandList(DXAccess::GetCurrentBackBufferIndex());

	ComPtr<ID3D12Resource> intermediateResource;
	UpdateBufferResource(commandList, &destinationResource, &intermediateResource, numberOfElements, elementSize, bufferData, flags);

	directCommands->ExecuteCommandList(DXAccess::GetCurrentBackBufferIndex());
	directCommands->Signal();
	directCommands->WaitForFenceValue(DXAccess::GetCurrentBackBufferIndex());

	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();

	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = destinationResource->GetGPUVirtualAddress();
	desc.SizeInBytes = bufferSize;

	CD3DX12_CPU_DESCRIPTOR_HANDLE handle = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetCPUHandleAt(CBVIndex);
	device->CreateConstantBufferView(&desc, handle);
}

/// <summary>
/// First clears the passed Render Target & DepthStencil buffer
/// Depth Stencil only gets cleared when its passed along
/// </summary>
inline void BindAndClearRenderTarget(Window* window, CD3DX12_CPU_DESCRIPTOR_HANDLE* renderTarget, CD3DX12_CPU_DESCRIPTOR_HANDLE* depthStencil = nullptr, float* clearColor = nullptr)
{
	DXCommands* directCommands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT);
	ComPtr<ID3D12GraphicsCommandList2> commandList = directCommands->GetGraphicsCommandList();
	const float defaultClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	if(clearColor)
	{
		commandList->ClearRenderTargetView(*renderTarget, clearColor, 0, nullptr);
	}
	else
	{
		commandList->ClearRenderTargetView(*renderTarget, defaultClearColor, 0, nullptr);
	}
	

	if(depthStencil != nullptr)
	{
		commandList->ClearDepthStencilView(*depthStencil, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	commandList->RSSetViewports(1, &window->GetViewport());
	commandList->RSSetScissorRects(1, &window->GetScissorRect());
	commandList->OMSetRenderTargets(1, renderTarget, FALSE, depthStencil);
}