#include "Graphics/DXDescriptorHeap.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"

#include <cassert>

DXDescriptorHeap::DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags) 
	: descriptorCount(numberOfDescriptors)
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	D3D12_DESCRIPTOR_HEAP_DESC description = {};
	description.NumDescriptors = numberOfDescriptors;
	description.Type = type;
	description.Flags = flags;

	ThrowIfFailed(device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&descriptorHeap)));
	descriptorSize = device->GetDescriptorHandleIncrementSize(type);
}

ComPtr<ID3D12DescriptorHeap> DXDescriptorHeap::Get()
{
	return descriptorHeap;
}

ID3D12DescriptorHeap* DXDescriptorHeap::GetAddress()
{
	return descriptorHeap.Get();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DXDescriptorHeap::GetCPUHandleAt(unsigned int index)
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(descriptorHeap->GetCPUDescriptorHandleForHeapStart(), index, descriptorSize);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DXDescriptorHeap::GetGPUHandleAt(unsigned int index)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(descriptorHeap->GetGPUDescriptorHandleForHeapStart(), index, descriptorSize);
}

unsigned int DXDescriptorHeap::GetNextAvailableIndex()
{
	if(currentDescriptorIndex >= descriptorCount)
	{
		assert(false && "Descriptor count within heap has been exceeded!");
		return 0;
	}

	unsigned int index = currentDescriptorIndex;
	currentDescriptorIndex++;
	return index;
}
