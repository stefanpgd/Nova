#pragma once
#define WIN32_LEAN_AND_MEAN // reduces the amount of headers included from Windows.h
#include <Windows.h>
#include <string>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dx12.h>


/// <summary>
/// The window is responsible for creating the window and the swapchain.
/// The window also manages presenting and resizing.
/// </summary>
class Window
{
public:
	Window(std::wstring windowName, unsigned int windowWidth, unsigned int windowHeight, 
		ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandQueue> commandQueue);

	void Present();
	void Resize();

	HWND GetHWND();
	unsigned int GetCurrentBackBufferIndex();
	ComPtr<ID3D12Resource> GetCurrentBackBuffer();
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV();

private:
	void SetupWindow(HINSTANCE hInst);
	void CreateSwapChain();
	void CreateRTVDescriptorHeap();
	void UpdateRenderTargetViews();

public:
	static constexpr unsigned int BackBufferCount = 3;

private:
	// Temporary reference to device and command queue until we've global accessors //
	ComPtr<ID3D12Device2> device;
	ComPtr<ID3D12CommandQueue> commandQueue;

	// Window settings //
	std::wstring windowName;
	unsigned int windowWidth;
	unsigned int windowHeight;
	RECT windowRect;
	HWND hWnd;

	bool vSync = true; // present frames at the same interval as the screen's refresh rate
	bool tearingSupported = false;
	bool fullscreen = false;

	// SwapChain //
	ComPtr<IDXGISwapChain4> swapChain;
	ComPtr<ID3D12Resource> backBuffers[BackBufferCount];
	unsigned int currentBackBufferIndex;

	ComPtr<ID3D12DescriptorHeap> RTVDescriptorHeap;
	UINT RTVDescriptorSize;
};