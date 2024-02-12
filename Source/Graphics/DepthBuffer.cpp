#include "Graphics/DepthBuffer.h"

#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"
#include <d3dx12.h>

DepthBuffer::DepthBuffer(unsigned int width, unsigned int height) : width(width), height(height)
{
	DXDescriptorHeap* DSVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	depthDSVIndex = DSVHeap->GetNextAvailableIndex();
	depthSRVIndex = SRVHeap->GetNextAvailableIndex();

	Resize(width, height);
}

void DepthBuffer::Resize(unsigned int width, unsigned int height)
{
	this->width = width;
	this->height = height;

	depthBuffer.Reset();

	const DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT;

	// 2. Create a new Depth Buffer //
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	DXDescriptorHeap* DSVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = format;
	clearValue.DepthStencil = { 1.0f, 0 };

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC depthDescription = CD3DX12_RESOURCE_DESC::Tex2D(format,
		width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&depthDescription, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &clearValue, IID_PPV_ARGS(&depthBuffer)));

	// 3. Create Depth-Stencil view //
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = format;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, DSVHeap->GetCPUHandleAt(depthDSVIndex));

	// 4. Create Shader Resource View //
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	
	device->CreateShaderResourceView(depthBuffer.Get(), &srvDesc, SRVHeap->GetCPUHandleAt(depthSRVIndex));
}

ComPtr<ID3D12Resource> DepthBuffer::GetResource()
{
	return depthBuffer;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDSV()
{
	DXDescriptorHeap* DSVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	return DSVHeap->GetCPUHandleAt(depthDSVIndex);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DepthBuffer::GetSRV()
{
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return SRVHeap->GetGPUHandleAt(depthSRVIndex);
}

unsigned int DepthBuffer::GetWidth()
{
	return width;
}

unsigned int DepthBuffer::GetHeight()
{
	return height;
}
