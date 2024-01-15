#pragma once

#if defined(max)
#undef max
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>
#include <DirectXMath.h>

using namespace DirectX;
typedef DirectX::XMMATRIX matrix;
typedef DirectX::XMFLOAT3 float3;

#include "DXAccess.h"
#include "DXCommands.h"

inline void ThrowIfFailed(HRESULT hr)
{
	if(FAILED(hr))
	{
		throw std::exception();
	}
}

inline void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	ComPtr<ID3D12GraphicsCommandList2> commandList = DXAccess::GetCommands()->GetCommandList();
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource, before, after);
	commandList->ResourceBarrier(1, &barrier);
}