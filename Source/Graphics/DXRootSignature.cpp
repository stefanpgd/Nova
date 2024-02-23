#include "Graphics/DXRootSignature.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"
#include "Utilities/Logger.h"

#include <cassert>

DXRootSignature::DXRootSignature(CD3DX12_ROOT_PARAMETER1* rootParameters,
	const unsigned int numberOfParameters, D3D12_ROOT_SIGNATURE_FLAGS flags)
{
	// Check if version 1.1 is supported for our Root Signature //
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	int result = DXAccess::GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData));
	if(result < 0) // FAILED
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Create Root Signature //
	// PLACEHOLDER //
	const CD3DX12_STATIC_SAMPLER_DESC sampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(numberOfParameters, rootParameters, 1, &sampler, flags);

	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> rootSignatureError;
	D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &rootSignatureError);

	if(!rootSignatureError == NULL)
	{
		std::string buffer = std::string((char*)rootSignatureError->GetBufferPointer());
		LOG(Log::MessageType::Error, buffer);
		assert(false && "Compilation of shader failed, read console for errors.");
	}

	ThrowIfFailed(DXAccess::GetDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

ComPtr<ID3D12RootSignature> DXRootSignature::Get()
{
	return rootSignature;
}

ID3D12RootSignature* DXRootSignature::GetAddress()
{
	return rootSignature.Get();
}