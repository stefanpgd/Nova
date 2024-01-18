#include "Window.h"
#include "DXUtilities.h"
#include "DXAccess.h"
#include "DXDescriptorHeap.h"
#include "DXCommands.h"

#include <cassert>

Window::Window(const std::wstring& applicationName, unsigned int windowWidth, unsigned int windowHeight) :
	windowName(applicationName), windowWidth(windowWidth), windowHeight(windowHeight)
{
	rtvHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Check for Tearing Support //
	ComPtr<IDXGIFactory5> factory5;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory5)));

	if(FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupported, sizeof(tearingSupported))))
	{
		tearingSupported = false;
	}

	SetupWindow();

	CreateSwapChain();
	UpdateRenderTargetViews();

	scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight));

	ShowWindow(windowHandle, SW_SHOW);
}

void Window::Present()
{
	UINT syncInterval = vSync ? 1 : 0;
	UINT presentFlags = tearingSupported && !vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(swapChain->Present(syncInterval, presentFlags));
}

void Window::Resize()
{
	RECT clientRect = {};
	GetClientRect(windowHandle, &clientRect);

	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;

	if(windowWidth != width || windowHeight != height)
	{
		windowWidth = width > 1 ? width : 1;
		windowWidth = height > 1 ? height : 1;

		for(int i = 0; i < BackBufferCount; ++i)
		{
			// Any references to the back buffers must be released
			// before the swap chain can be resized.
			backBuffers[i].Reset();
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(swapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(swapChain->ResizeBuffers(BackBufferCount, windowWidth, windowHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

		UpdateRenderTargetViews();

		viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight));
	}
}

unsigned int Window::GetCurrentBackBufferIndex()
{
	return swapChain->GetCurrentBackBufferIndex();
}

ComPtr<ID3D12Resource> Window::GetCurrentBackBuffer()
{
	return backBuffers[GetCurrentBackBufferIndex()];
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentBackBufferRTV()
{
	return rtvHeap->GetCPUHandleAt(GetCurrentBackBufferIndex());
}

HWND Window::GetHWND()
{
	return windowHandle;
}

unsigned int Window::GetWindowWidth()
{
	return windowWidth;
}

unsigned int Window::GetWindowHeight()
{
	return windowHeight;
}

const D3D12_VIEWPORT& Window::GetViewport()
{
	return viewport;
}

const D3D12_RECT& Window::GetScissorRect()
{
	return scissorRect;
}

void Window::SetupWindow()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	int windowX = std::max(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max(0, (screenHeight - windowHeight) / 2);

	windowHandle = ::CreateWindowExW(NULL, windowName.c_str(), windowName.c_str(), WS_OVERLAPPEDWINDOW,
		windowX, windowY, windowWidth, windowHeight, NULL, NULL, hInstance, nullptr);

	assert(windowHandle && "Failed to create window handle");

	GetWindowRect(windowHandle, &windowRect);
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

	ComPtr<ID3D12CommandQueue> commandQueue = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetCommandQueue();
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, &swapChain1));
	ThrowIfFailed(factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(swapChain1.As(&swapChain));
}

void Window::UpdateRenderTargetViews()
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();

	for(int i = 0; i < BackBufferCount; i++)
	{
		ComPtr<ID3D12Resource> backBuffer;
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUHandleAt(i);

		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		backBuffers[i] = backBuffer;
	}
}