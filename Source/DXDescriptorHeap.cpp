#include "DXDescriptorHeap.h"
#include "DXDevice.h"
#include "DXAccess.h"
#include "DXUtilities.h"
#include <d3dx12.h>
#include <cassert>

DXDescriptorHeap::DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int amountOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
	D3D12_DESCRIPTOR_HEAP_DESC description = {};
	description.Type = type;
	description.NumDescriptors = amountOfDescriptors;
	description.Flags = flags;

	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	ThrowIfFailed(device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&heap)));

	maxDescriptors = amountOfDescriptors;
	descriptorByteSize = device->GetDescriptorHandleIncrementSize(type);
}

ComPtr<ID3D12DescriptorHeap> DXDescriptorHeap::Get()
{
	return heap;
}

ID3D12DescriptorHeap* DXDescriptorHeap::GetAddress()
{
	return heap.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DXDescriptorHeap::GetCPUHandleAt(unsigned int index)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(heap->GetCPUDescriptorHandleForHeapStart(), index, descriptorByteSize);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DXDescriptorHeap::GetGPUHandleAt(unsigned int index)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(heap->GetGPUDescriptorHandleForHeapStart(), index, descriptorByteSize);
}

unsigned int DXDescriptorHeap::GetNextAvailableIndex()
{
	if(currentDescriptorIndex >= maxDescriptors)
	{
		assert(false && "Descriptor count within heap has been exceeded!");
		return 0;
	}

	unsigned int index = currentDescriptorIndex;
	currentDescriptorIndex++;
	return index;
}