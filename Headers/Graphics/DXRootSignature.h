#pragma once

#include "d3d12.h"
#include "d3dx12.h"
#include "wrl.h"
using namespace Microsoft::WRL;

class DXRootSignature
{
public:
	DXRootSignature(CD3DX12_ROOT_PARAMETER1* rootParameters,
		const unsigned int numberOfParameters, D3D12_ROOT_SIGNATURE_FLAGS flags);

	ComPtr<ID3D12RootSignature> Get();
	ID3D12RootSignature* GetAddress();

private:
	ComPtr<ID3D12RootSignature> rootSignature;
};