#define WIN32_LEAN_AND_MEAN // reduces the amount of headers included from Windows.h
#include <Windows.h>

// Within windows.h CreateWindow is already defined, so we overwrite it with our own
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows runtime library, needed for ComPtr template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 Headers
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include <d3dx12.h>

// STL Headers
#include <iostream>
#include <algorithm>
#include <cassert>
#include <chrono>

// From DXSampleHelper.h
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

bool g_IsInitialized = false;
bool g_UseWarp = false;
const unsigned int g_NumFrames = 3; // amount of back buffers for swap chain

unsigned int g_WindowWidth = 1280;
unsigned int g_WindowHeight = 720;

HWND g_hWnd;
RECT g_WindowRect;

ComPtr<ID3D12Device2> g_Device;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<IDXGISwapChain4> g_SwapChain;
ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
ComPtr<ID3D12GraphicsCommandList> g_CommandList;
ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
UINT g_RTVDescriptorSize;
UINT g_CurrentBackBufferIndex;

int main()
{
	printf("Hello World\n");
	return 0;
}