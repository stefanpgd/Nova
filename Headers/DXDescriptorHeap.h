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

	ComPtr<ID3D12DescriptorHeap> Get();
	ID3D12DescriptorHeap* GetAddress();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandleAt(unsigned int index);
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandleAt(unsigned int index);

	unsigned int GetNextAvailableIndex();

private:
	ComPtr<ID3D12DescriptorHeap> heap;

	unsigned int currentDescriptorIndex = 0;
	unsigned int maxDescriptors;
	unsigned int descriptorByteSize;
};