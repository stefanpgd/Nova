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

#include <iostream>

int main()
{
	printf("Hello World\n");
	return 0;
}