#pragma once

#if defined(max)
#undef max
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>
#include <cassert>

#include "DXAccess.h"
#include "DXCommands.h"

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}

inline void UpdateBufferResource(ID3D12Resource** destinationResource, ID3D12Resource** intermediateBuffer, size_t numberOfElements, size_t elementSize, const void* bufferData)
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	DXCommands* commands = DXAccess::GetCommands();

	if (!bufferData)
	{
		assert(false && "Buffer that is trying to be uploaded to GPU memory is NULL");
	}

	// Size of the entire buffer in bytes & properties
	size_t bufferSize = elementSize * numberOfElements;
	CD3DX12_RESOURCE_DESC buffer = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
	CD3DX12_HEAP_PROPERTIES destinationHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_HEAP_PROPERTIES intermediateHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Create a commited resource for the GPU resource
	// This is used to create a resource and an implicit heap that is large enough to store the given resource. This `destinationResource` is also mapped to the implicit heap
	ThrowIfFailed(device->CreateCommittedResource(&destinationHeap, D3D12_HEAP_FLAG_NONE, &buffer,
		D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(destinationResource)));

	// Though the heap is created, nothing has been uploaded yet
	// We create another resource, this time on the GPU's Upload Heap.
	// This heap is optimized to receive a CPU to GPU write-once only call
	// Then once it's on the GPU, we can transfer the data from the Upload Heap to the Default heap
	ThrowIfFailed(device->CreateCommittedResource(&intermediateHeap,
		D3D12_HEAP_FLAG_NONE,
		&buffer,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(intermediateBuffer)));

	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = bufferData;
	subresourceData.RowPitch = bufferSize;
	subresourceData.SlicePitch = bufferSize; // Double check if 1 works here as well?

	// Using the info from the subresource data struct, the data will now be copied over to the Upload heap and then the Default Heap to the destination resource.
	UpdateSubresources(commands->GetCommandList().Get(), *destinationResource, *intermediateBuffer, 0, 0, 1, &subresourceData);
}