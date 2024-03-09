#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class HDRi 
{
public:
	HDRi(const std::string& filePath);

	int GetSRVIndex();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRV();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPULocation();
	ComPtr<ID3D12Resource> GetResource();

private:
	void UploadData(float* data, int width, int height);

private:
	ComPtr<ID3D12Resource> hdriResource;
	int srvIndex = 0;
};