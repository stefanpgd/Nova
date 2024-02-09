#include "Graphics/Texture.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXAccess.h"
#include <stb_image.h>

// TODO: Learn about mipmapping with DX12 
// TODO: Support texture loading through filepath
Texture::Texture(tinygltf::Model& model, tinygltf::Texture& texture)
{
	tinygltf::Image& image = model.images[texture.source];

	UploadData(image.image.data(), image.width, image.height);
}

Texture::Texture(const std::string& filePath)
{
	int width;
	int height; 
	int channels;

	stbi_set_flip_vertically_on_load(true);
	unsigned char* buffer = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
	stbi_set_flip_vertically_on_load(false);

	if (buffer == NULL)
	{
		LOG(Log::MessageType::Error, "Unsuccesful with loading: " + filePath);
		assert(false);
	}

	UploadData(buffer, width, height);
}

Texture::Texture(unsigned char* data, int width, int height)
{
	UploadData(data, width, height);
}

int Texture::GetSRVIndex()
{
	return srvIndex;
}

D3D12_GPU_VIRTUAL_ADDRESS Texture::GetGPULocation()
{
	return textureResource->GetGPUVirtualAddress();
}

ComPtr<ID3D12Resource> Texture::GetResource()
{
	return textureResource;
}

void Texture::UploadData(unsigned char* data, int width, int height)
{
	D3D12_RESOURCE_DESC description = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	description.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_SUBRESOURCE_DATA subresource;
	subresource.pData = data;
	subresource.RowPitch = width * sizeof(unsigned int);

	ComPtr<ID3D12Resource> intermediateTexture;
	UploadPixelShaderResource(textureResource, intermediateTexture, description, subresource);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvIndex = SRVHeap->GetNextAvailableIndex();

	DXAccess::GetDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, SRVHeap->GetCPUHandleAt(srvIndex));
}