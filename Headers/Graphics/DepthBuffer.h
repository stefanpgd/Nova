#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DepthBuffer
{
public:
	DepthBuffer(unsigned int width, unsigned int height);

	void Resize(unsigned int width, unsigned int height);

	ComPtr<ID3D12Resource> GetResource();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDSV();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRV();
	unsigned int GetWidth();
	unsigned int GetHeight();

private:
	unsigned int width;
	unsigned int height;

	ComPtr<ID3D12Resource> depthBuffer;
	int depthDSVIndex;
	int depthSRVIndex;
};