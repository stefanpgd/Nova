#pragma once

class DXDevice;
class DXCommands;

#include <wrl.h>
#include <d3d12.h>
using namespace Microsoft::WRL;

namespace DXAccess
{
	ComPtr<ID3D12Device2> GetDevice();
	DXCommands* GetCommands();
}