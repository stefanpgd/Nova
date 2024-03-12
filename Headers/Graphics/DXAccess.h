#pragma once

class DXDevice;
class DXCommands;
class DXDescriptorHeap;
class Texture;
class Window;

#include <wrl.h>
#include <d3d12.h>
using namespace Microsoft::WRL;

namespace DXAccess
{
	DXCommands* GetCommands(D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12Device2> GetDevice();
	DXDescriptorHeap* GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);
	Window* GetWindow();

	unsigned int GetCurrentBackBufferIndex();
	Texture* GetDefaultTexture();

}