#pragma once

#include <wrl.h>
#include <d3d12.h>
#include <d3dx12.h>
using namespace Microsoft::WRL;

class DXDevice
{
public:
	DXDevice();

	ComPtr<ID3D12Device2> Get();
	ID3D12Device2* GetAddress();

private:
	void DebugLayer();
	void SetupMessageSeverities();

	ComPtr<ID3D12Device2> device;
};