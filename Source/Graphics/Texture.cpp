#include "Graphics/Texture.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXAccess.h"

// TODO: Learn about mipmapping with DX12 
Texture::Texture(tinygltf::Model& model, tinygltf::Texture& texture)
{
	tinygltf::Image& image = model.images[texture.source];

	D3D12_RESOURCE_DESC description = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, image.width, image.height);
	
	D3D12_SUBRESOURCE_DATA subresource;
	subresource.pData = image.image.data();
	subresource.RowPitch = image.width * sizeof(unsigned int);
	
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

int Texture::GetSRVIndex()
{
	return srvIndex;
}