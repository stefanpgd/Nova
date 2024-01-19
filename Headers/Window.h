#pragma once
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>

class DXDescriptorHeap;

class Window
{
public:
	Window(const std::wstring& applicationName, unsigned int windowWidth, unsigned int windowHeight);

	void Present();
	void Resize();

	unsigned int GetCurrentBackBufferIndex();
	ComPtr<ID3D12Resource> GetCurrentBackBuffer();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV();

	HWND GetHWND();
	unsigned int GetWindowWidth();
	unsigned int GetWindowHeight();
	const D3D12_VIEWPORT& GetViewport();
	const D3D12_RECT& GetScissorRect();

private:
	void SetupWindow();
	void CreateSwapChain();

	void UpdateRenderTargets();
	void UpdateDepthBuffer();

public:
	static const unsigned int BackBufferCount = 3;

private:
	// Window Settings //
	std::wstring windowName;
	unsigned int windowWidth;
	unsigned int windowHeight;
	HWND windowHandle;
	RECT windowRect;

	bool vSync = false;
	bool tearingSupported = true;
	bool fullscreen = false;

	// Swap Chain //
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource> backBuffers[BackBufferCount];
	DXDescriptorHeap* rtvHeap;

	// Depth Buffer //
	ComPtr<ID3D12Resource> depthBuffer;

	// Rasterizer Objects //
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
};