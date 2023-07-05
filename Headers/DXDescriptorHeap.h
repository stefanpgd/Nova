#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>

using namespace Microsoft::WRL;

class DXDescriptorHeap
{
public:
	DXDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int amountOfDescriptors,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	void GetCPUHandleAt(unsigned int index);
	void GetGPUHandleAt(unsigned int index);

	unsigned int GetNextAvailableHandle();

private:
	ComPtr<ID3D12DescriptorHeap> heap;

	unsigned int maxDescriptors;
	unsigned int descriptorByteSize;
};