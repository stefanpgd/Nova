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
class Texture;

class Window
{
public:
	Window(const std::wstring& applicationName, unsigned int windowWidth, unsigned int windowHeight);

	void Present();
	void Resize();

	unsigned int GetCurrentBackBufferIndex();

	// Render Targets are back buffers used to draw the scene into //
	ComPtr<ID3D12Resource> GetCurrentRenderBuffer();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderRTV();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetCurrentRenderSRV();

	// Screen Buffers are back-buffers of the Swap Chain //
	ComPtr<ID3D12Resource> GetCurrentScreenBuffer();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentScreenRTV();

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthDSV();

	HWND GetHWND();
	unsigned int GetWindowWidth();
	unsigned int GetWindowHeight();
	const D3D12_VIEWPORT& GetViewport();
	const D3D12_RECT& GetScissorRect();

private:
	void SetupWindow();
	void CreateSwapChain();

	void UpdateRenderBuffers();
	void UpdateScreenBuffers();
	void UpdateDepthBuffer();

public:
	static const unsigned int BackBufferCount = 3;

	// PLACEHOLDER //
	// Attempt at using backbuffers as textures //

private:
	// Window Settings //
	std::wstring windowName;
	unsigned int windowWidth;
	unsigned int windowHeight;
	HWND windowHandle;
	RECT windowRect;

	bool vSync = true;
	bool tearingSupported = true;
	bool fullscreen = false;

	// Render Buffers //
	Texture* renderBuffers[BackBufferCount];
	int renderBufferRTVs[BackBufferCount];
	int renderBufferSRVs[BackBufferCount];

	// Screen Buffers //
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource> screenBuffers[BackBufferCount];
	int screenBufferRTVs[BackBufferCount];

	// Depth Buffer //
	ComPtr<ID3D12Resource> depthBuffer;
	int depthDSVIndex;

	// Rasterizer Objects //
	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;
};