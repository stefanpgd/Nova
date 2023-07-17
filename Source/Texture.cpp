#include "Texture.h"

#include "DXDevice.h"
#include "DXCommands.h"
#include "DXAccess.h"
#include "DXUtilities.h"
#include "DXDescriptorHeap.h"
#include <stb_image.h>
#include <d3dx12.h>

Texture::Texture(std::string filePath)
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	DXCommands* commands = DXAccess::GetCommands();
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Load image into local memory //
	imageBuffer = stbi_load(filePath.c_str(), &width, &height, &channels, 4);

	// Describe that we want to use the regular memory heap on the GPU, and how big our
	// resource will using the Tex2D helper from d3dx12.h 
	D3D12_HEAP_PROPERTIES properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC description = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height);
	
	// Allocate GPU memory, passing `texture` as our reference object to get to this memory //
	ThrowIfFailed(device->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE,
		&description, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&texture)));

	// Setup which data we would like to push from CPU to GPU memory // 
	D3D12_SUBRESOURCE_DATA subresource = {};
	subresource.pData = imageBuffer;
	subresource.RowPitch = width * sizeof(UINT);
	subresource.SlicePitch = width * height * sizeof(UINT);

	// Upload the data to the 'Upload Heap' then to the 'Deafult Heap'. While doing so, we transition the
	// resource from 'Common data' -> 'Copy destination data' and finally to 'Pixel shader resource'
	UploadAndTransitionResource(texture, subresource, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
	srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Texture2D.MipLevels = 1;

	// With the data within the commited resource, together with a description of how to should be interpreted,
	// we can make an SRV that can be used by our shader pipeline.
	textureSRVIndex = SRVHeap->GetNextAvailableIndex();
	device->CreateShaderResourceView(texture.Get(), &srv, SRVHeap->GetCPUHandleAt(textureSRVIndex));

	stbi_image_free(imageBuffer);
}

UINT Texture::GetDescriptorIndex()
{
	return textureSRVIndex;
}
