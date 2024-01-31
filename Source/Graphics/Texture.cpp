#include "Graphics/Texture.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXAccess.h"

// TODO: learn about mipmapping with DX12
Texture::Texture(tinygltf::Model& model, tinygltf::Texture& texture)
{
	tinygltf::Image& image = model.images[texture.source];

	unsigned int imageWidth = image.width;
	unsigned int imageHeight = image.height;
	unsigned int objectSize = image.bits * image.component;

	// 1. CREATE RESOURCE ON GPU //
	D3D12_RESOURCE_DESC description = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R8G8B8A8_UNORM, imageWidth, imageHeight);

	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	D3D12_HEAP_PROPERTIES properties =
		CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(device->CreateCommittedResource(
		&properties, D3D12_HEAP_FLAG_NONE, &description, D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(&textureResource)));

	// 2. GET READY TO UPLOAD RESOURCE ON GPU //
	D3D12_SUBRESOURCE_DATA subresource;
	subresource.pData = image.image.data();
	subresource.RowPitch = imageWidth * sizeof(unsigned int);
	subresource.SlicePitch = imageWidth * imageHeight * sizeof(unsigned int);

	DXCommands* copyCommands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_COPY);
	ComPtr<ID3D12GraphicsCommandList2> copyCommandList = copyCommands->GetGraphicsCommandList();
	copyCommands->Flush();
	copyCommands->ResetCommandList();

	ComPtr<ID3D12Resource> intermediateTexture;
	UploadSRVTest(copyCommandList, textureResource.Get(), intermediateTexture.Get(), subresource);

	copyCommands->ExecuteCommandList(DXAccess::GetCurrentBackBufferIndex());
	copyCommands->Signal();
	copyCommands->WaitForFenceValue(DXAccess::GetCurrentBackBufferIndex());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	srvIndex = SRVHeap->GetNextAvailableIndex();

	device->CreateShaderResourceView(textureResource.Get(), &srvDesc, SRVHeap->GetCPUHandleAt(srvIndex));
}