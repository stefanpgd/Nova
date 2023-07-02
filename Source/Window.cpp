#include "Window.h"
#include "Utilities.h"
#include "resource.h"

#include <cassert>
#include <algorithm>

Window::Window(std::wstring windowName, unsigned int windowWidth, unsigned int windowHeight, 
	ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandQueue> commandQueue) :
	windowName(windowName), windowWidth(windowWidth), windowHeight(windowHeight), device(device), commandQueue(commandQueue)
{
	// Check for Tearing Support //
	ComPtr<IDXGIFactory5> factory5;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory5)));

	if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupported, sizeof(tearingSupported))))
	{
		tearingSupported = false;
	}

	HINSTANCE hInstance = GetModuleHandle(NULL);

	SetupWindow(hInstance);
	GetWindowRect(hWnd, &windowRect);

	CreateSwapChain();
	CreateRTVDescriptorHeap();
	UpdateRenderTargetViews();

	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

	ShowWindow(hWnd, SW_SHOW);
}

void Window::Present()
{
	UINT syncInterval = vSync ? 1 : 0;
	UINT presentFlags = tearingSupported && !vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(swapChain->Present(syncInterval, presentFlags));

	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
}

void Window::Resize()
{
	RECT clientRect = {};
	GetClientRect(hWnd, &clientRect);

	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;

	if (windowWidth != width || windowHeight != height)
	{
		windowWidth = width > 1 ? width : 1;
		windowWidth = height > 1 ? height : 1;

		for (int i = 0; i < BackBufferCount; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			backBuffers[i].Reset();
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(swapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(swapChain->ResizeBuffers(BackBufferCount, windowWidth, windowHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
		UpdateRenderTargetViews();
	}
}

unsigned int Window::GetCurrentBackBufferIndex()
{
	return currentBackBufferIndex;
}

ComPtr<ID3D12Resource> Window::GetCurrentBackBuffer()
{
	return backBuffers[currentBackBufferIndex];
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentBackBufferRTV()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex, RTVDescriptorSize);
	return rtv;
}

unsigned int Window::GetWindowWidth()
{
	return windowWidth;
}

unsigned int Window::GetWindowHeight()
{
	return windowHeight;
}

HWND Window::GetHWND()
{
	return hWnd;
}

void Window::SetupWindow(HINSTANCE hInst)
{
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

	HWND windowHandle = ::CreateWindowExW(NULL, windowName.c_str(), windowName.c_str(), WS_OVERLAPPEDWINDOW,
		windowX, windowY, windowWidth, windowHeight, NULL, NULL, hInst, nullptr);

	assert(windowHandle && "Failed to create window");
	hWnd = windowHandle;
}

void Window::CreateSwapChain()
{
	ComPtr<IDXGIFactory4> factory;

	UINT factoryFlags = 0;
#if defined(_DEBUG)
	factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&factory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = windowWidth;
	swapChainDesc.Height = windowHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = BackBufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
	ThrowIfFailed(factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&swapChain));
}

void Window::CreateRTVDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC description = {};
	description.NumDescriptors = BackBufferCount;
	description.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ThrowIfFailed(device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&RTVDescriptorHeap)));
	RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

// The swapchain already makes the texture resources for us, so we don't have to create them.
// Instead we just update the RTVs based on the new descriptions of the resources (back buffer textures) in the swapchain.
void Window::UpdateRenderTargetViews()
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < BackBufferCount; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;

		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		backBuffers[i] = backBuffer;
		rtvHandle.Offset(RTVDescriptorSize);
	}
}

