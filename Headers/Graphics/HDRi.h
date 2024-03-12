#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class DXRootSignature;
class DXPipeline;

class HDRI 
{
public:
	HDRI(const std::string& filePath);

	int GetSRVIndex();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRV();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPULocation();
	ComPtr<ID3D12Resource> GetResource();

	void HDRIDebugWindow();

private:
	void UploadBuffer(float* data, int width, int height, 
		ComPtr<ID3D12Resource>& resource, int& index);

	void CreatePipeline();

	void ConvoluteHDRI();

private:
	ComPtr<ID3D12Resource> hdriResource;
	ComPtr<ID3D12Resource> irradianceResource;

	DXRootSignature* rootSignature;
	DXPipeline* pipeline;

	int hdriIndex = 0;
	int irradianceIndex = 0;
};