#include "Graphics/DepthBuffer.h"

#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"
#include <d3dx12.h>

DepthBuffer::DepthBuffer(unsigned int width, unsigned int height) : width(width), height(height)
{
	DXDescriptorHeap* DSVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	depthDSVIndex = DSVHeap->GetNextAvailableIndex();

	Resize(width, height);
}

void DepthBuffer::Resize(unsigned int width, unsigned int height)
{
	this->width = width;
	this->height = height;

	depthBuffer.Reset();

	// 2. Create a new Depth Buffer //
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	DXDescriptorHeap* dsvHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil = { 1.0f, 0 };

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC depthDescription = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
		width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&depthDescription, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthBuffer)));

	// 3. Create Depth-Stencil view //
	D3D12_DEPTH_STENCIL_VIEW_DESC DSV;
	DSV.Format = DXGI_FORMAT_D32_FLOAT;
	DSV.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	DSV.Texture2D.MipSlice = 0;
	DSV.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthBuffer.Get(), &DSV, dsvHeap->GetCPUHandleAt(depthDSVIndex));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDSV()
{
	DXDescriptorHeap* DSVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	return DSVHeap->GetCPUHandleAt(depthDSVIndex);
}
