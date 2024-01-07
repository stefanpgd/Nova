#pragma once

class DXDevice;
class DXCommands;
class DXDescriptorHeap;

#include <wrl.h>
#include <d3d12.h>
using namespace Microsoft::WRL;

namespace DXAccess
{
	DXCommands* GetCommands();
	ComPtr<ID3D12Device2> GetDevice();
	DXDescriptorHeap* GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);

	unsigned int GetCurrentBackBufferIndex();
}