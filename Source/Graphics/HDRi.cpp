#include "Graphics/HDRi.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXAccess.h"
#include <stb_image.h>

HDRi::HDRi(const std::string& filePath)
{
	int width;
	int height;
	int channels;

	stbi_set_flip_vertically_on_load(true);

	// Instead of 8 bits, we store 32 bits per channel with HDRIs
	float* buffer = stbi_loadf(filePath.c_str(), &width, &height, &channels, 4);
	stbi_set_flip_vertically_on_load(false);

	if(buffer == NULL)
	{
		LOG(Log::MessageType::Error, "Unsuccesful with loading: " + filePath);
		assert(false);
	}

	UploadData(buffer, width, height);

	stbi_image_free(buffer);
}

int HDRi::GetSRVIndex()
{
	return srvIndex;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE HDRi::GetSRV()
{
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return SRVHeap->GetGPUHandleAt(srvIndex);
}

D3D12_GPU_VIRTUAL_ADDRESS HDRi::GetGPULocation()
{
	return hdriResource->GetGPUVirtualAddress();
}

ComPtr<ID3D12Resource> HDRi::GetResource()
{
	return hdriResource;
}

void HDRi::UploadData(float* data, int width, int height)
{
	D3D12_RESOURCE_DESC description = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32G32B32A32_FLOAT, width, height);
	description.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_SUBRESOURCE_DATA subresource;
	subresource.pData = data;
	subresource.RowPitch = width * (sizeof(float) * 4);

	ComPtr<ID3D12Resource> intermediate;
	UploadPixelShaderResource(hdriResource, intermediate, description, subresource);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvIndex = SRVHeap->GetNextAvailableIndex();

	DXAccess::GetDevice()->CreateShaderResourceView(hdriResource.Get(), &srvDesc, SRVHeap->GetCPUHandleAt(srvIndex));
}