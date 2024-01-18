#pragma once

#if defined(max)
#undef max
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>
#include <DirectXMath.h>

using namespace DirectX;
typedef DirectX::XMMATRIX matrix;
typedef DirectX::XMFLOAT3 float3;

#include "DXAccess.h"
#include "DXCommands.h"

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
		assert(false && "Buffer data is NOT valid!");
		return;
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