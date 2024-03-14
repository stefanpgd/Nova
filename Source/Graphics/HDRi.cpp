#include "Graphics/HDRI.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXAccess.h"

#include <stb_image.h>

#include <imgui.h>
#include <glm.hpp>

HDRI::HDRI(const std::string& filePath)
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

	this->width = width;
	this->height = height;

	// 1. Upload Environment map
	UploadBuffer(buffer, width, height, hdriResource, hdriIndex);
	stbi_image_free(buffer);

	// 2. Allocate resource with matching dimensions to read/write to
	float* buff = new float[width * height * sizeof(float)];
	UploadBuffer(buff, width, height, irradianceResource, irradianceIndex);
	delete[] buff;

	DXDescriptorHeap* rtvHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	irradianceRTVIndex = rtvHeap->GetNextAvailableIndex();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUHandleAt(irradianceRTVIndex);
	DXAccess::GetDevice()->CreateRenderTargetView(irradianceResource.Get(), nullptr, rtvHandle);
}

int HDRI::GetHDRiSRVIndex()
{
	return hdriIndex;
}

int HDRI::GetIrradianceSRVIndex()
{
	return irradianceIndex;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE HDRI::GetHDRISRVHandle()
{
	DXDescriptorHeap* srvHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return srvHeap->GetGPUHandleAt(hdriIndex);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE HDRI::GetIraddianceRTVHandle()
{
	DXDescriptorHeap* rtvHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	return rtvHeap->GetCPUHandleAt(irradianceRTVIndex);
}

ComPtr<ID3D12Resource> HDRI::GetHDRiResource()
{
	return hdriResource;
}

ComPtr<ID3D12Resource> HDRI::GetIrradianceResource()
{
	return irradianceResource;
}

static glm::vec3 testing = glm::vec3(0.0f, 0.0f, 0.0f);

void HDRI::HDRIDebugWindow()
{
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ImGui::Begin("HDRI View");

	ImGui::Text("HDRI - Regular");
	CD3DX12_GPU_DESCRIPTOR_HANDLE hdriHandle = SRVHeap->GetGPUHandleAt(hdriIndex);
	ImGui::Image((ImTextureID)hdriHandle.ptr, ImVec2(512, 256));

	ImGui::Separator();

	ImGui::Text("HDRI - Irradiance Capture");
	CD3DX12_GPU_DESCRIPTOR_HANDLE irradianceHandle = SRVHeap->GetGPUHandleAt(irradianceIndex);
	ImGui::Image((ImTextureID)irradianceHandle.ptr, ImVec2(512, 256));

	ImGui::DragFloat3("Tester", &testing.x, 0.001f); 
	testing = glm::normalize(testing);

	ImGui::End();
}

int HDRI::GetWidth()
{
	return width;
}

int HDRI::GetHeight()
{
	return height;
}

void HDRI::UploadBuffer(float* data, int width, int height, ComPtr<ID3D12Resource>& resource, int& index)
{
	D3D12_RESOURCE_DESC description = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32G32B32A32_FLOAT, width, height);
	description.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_SUBRESOURCE_DATA subresource;
	subresource.pData = data;
	subresource.RowPitch = width * (sizeof(float) * 4);

	ComPtr<ID3D12Resource> intermediate;
	UploadPixelShaderResource(resource, intermediate, description, subresource);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	index = SRVHeap->GetNextAvailableIndex();

	DXAccess::GetDevice()->CreateShaderResourceView(resource.Get(), &srvDesc, SRVHeap->GetCPUHandleAt(index));
}