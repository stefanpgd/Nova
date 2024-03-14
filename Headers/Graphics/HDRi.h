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

	int GetHDRiSRVIndex();
	int GetIrradianceSRVIndex();

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetHDRISRVHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetIraddianceRTVHandle();

	ComPtr<ID3D12Resource> GetHDRiResource();
	ComPtr<ID3D12Resource> GetIrradianceResource();

	void HDRIDebugWindow();

	int GetWidth();
	int GetHeight();

private:
	void UploadBuffer(float* data, int width, int height, 
		ComPtr<ID3D12Resource>& resource, int& index);

public:
	bool IsConvoluted = false;

private:
	ComPtr<ID3D12Resource> hdriResource;
	ComPtr<ID3D12Resource> irradianceResource;

	int hdriIndex = 0;
	int irradianceIndex = 0;

	int irradianceRTVIndex = 0;

	int width = 0;
	int height = 0;
};