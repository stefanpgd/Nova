#include "Renderer.h"
#include "DXUtilities.h"

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

// Windows runtime library, needed for ComPtr template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 Headers
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include "d3dx12.h"

Renderer::Renderer(const std::string& applicationName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	DebugLayer();
}

void Renderer::Render()
{
}

void Renderer::DebugLayer()
{
#if defined (_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}