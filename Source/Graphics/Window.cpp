#include "Graphics/Window.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXDescriptorHeap.h"
#include "Graphics/DXCommands.h"
#include "Graphics/Texture.h"

#include <cassert>

Window::Window(const std::wstring& applicationName, unsigned int windowWidth, unsigned int windowHeight) :
	windowName(applicationName), windowWidth(windowWidth), windowHeight(windowHeight)
{
	// Check for Tearing Support //
	ComPtr<IDXGIFactory5> factory5;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory5)));

	if(FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearingSupported, sizeof(tearingSupported))))
	{
		tearingSupported = false;
	}

	DXDescriptorHeap* RTVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DXDescriptorHeap* DSVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	// Get RTV indices for Screen & Render Buffers //
	for(int i = 0; i < BackBufferCount; i++)
	{
		renderBufferRTVs[i] = RTVHeap->GetNextAvailableIndex();
		screenBufferRTVs[i] = RTVHeap->GetNextAvailableIndex();
	}

	depthDSVIndex = DSVHeap->GetNextAvailableIndex();

	SetupWindow();
	CreateSwapChain();

	UpdateRenderBuffers();
	UpdateScreenBuffers();
	UpdateDepthBuffer();

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
		windowHeight = height > 1 ? height : 1;
		viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight));

		UpdateRenderBuffers();
		UpdateScreenBuffers();
		UpdateDepthBuffer();
	}
}

unsigned int Window::GetCurrentBackBufferIndex()
{
	return swapChain->GetCurrentBackBufferIndex();
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

void Window::UpdateRenderBuffers()
{
	// 1. Release old render buffers //
	for(int i = 0; i < BackBufferCount; i++)
	{
		delete renderBuffers[i];
	}

	// 2. Create new Render Buffers/Targets with Textures //
	int bufferSize = windowWidth * windowHeight * sizeof(unsigned int);
	unsigned char* buffer = new unsigned char[bufferSize];

	for(int i = 0; i < 3; i++)
	{
		renderBuffers[i] = new Texture(buffer, windowWidth, windowHeight);
	}

	delete[] buffer;

	// 3. Update Render Buffer RTVs & SRVs //
	DXDescriptorHeap* RTVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	DXDescriptorHeap* CBVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();

	for(int i = 0; i < BackBufferCount; i++)
	{
		// RTV //
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->GetCPUHandleAt(renderBufferRTVs[i]);
		ComPtr<ID3D12Resource> backBufferResource = renderBuffers[i]->GetResource();
		DXAccess::GetDevice()->CreateRenderTargetView(backBufferResource.Get(), nullptr, rtvHandle);

		// SRV //
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MipLevels = 1;

		renderBufferSRVs[i] = CBVHeap->GetNextAvailableIndex();
		ComPtr<ID3D12Resource> textureResource = renderBuffers[i]->GetResource();
		device->CreateShaderResourceView(textureResource.Get(), &srvDesc, CBVHeap->GetCPUHandleAt(renderBufferSRVs[i]));
	}
}

void Window::UpdateScreenBuffers()
{
	// 1. Release resources to free up memory //
	for(int i = 0; i < BackBufferCount; ++i)
	{
		// Any references to the back buffers must be released
		// before the swap chain can be resized.
		screenBuffers[i].Reset();
	}

	// 2. Resize swap chain ( screen buffers ) //
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ThrowIfFailed(swapChain->GetDesc(&swapChainDesc));
	ThrowIfFailed(swapChain->ResizeBuffers(BackBufferCount, windowWidth, windowHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

	// 3. Create new RTVs //
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	DXDescriptorHeap* RTVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	for(int i = 0; i < BackBufferCount; i++)
	{
		ComPtr<ID3D12Resource> backBuffer;
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle = RTVHeap->GetCPUHandleAt(screenBufferRTVs[i]);

		ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		screenBuffers[i] = backBuffer;
	}
}

void Window::UpdateDepthBuffer()
{
	// 1. Release resource //
	depthBuffer.Reset();

	// 2. Create a new Depth Buffer //
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	DXDescriptorHeap* dsvHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil = { 1.0f, 0 };

	CD3DX12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC depthDescription = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
		windowWidth, windowHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
		&depthDescription, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&depthBuffer)));

	// 3. Create Depth-Stencil view //
	D3D12_DEPTH_STENCIL_VIEW_DESC DSV;
	DSV.Format = DXGI_FORMAT_D32_FLOAT;
	DSV.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	DSV.Texture2D.MipSlice = 0;
	DSV.Flags = D3D12_DSV_FLAG_NONE;

	device->CreateDepthStencilView(depthBuffer.Get(), &DSV, dsvHeap->GetCPUHandleAt(depthDSVIndex));
}

#pragma region Getters
ComPtr<ID3D12Resource> Window::GetCurrentRenderBuffer()
{
	int index = GetCurrentBackBufferIndex();
	return renderBuffers[index]->GetResource();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentRenderRTV()
{
	DXDescriptorHeap* RTVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	int index = GetCurrentBackBufferIndex();
	return RTVHeap->GetCPUHandleAt(renderBufferRTVs[index]);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Window::GetCurrentRenderSRV()
{
	DXDescriptorHeap* SRVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	int index = GetCurrentBackBufferIndex();
	return SRVHeap->GetGPUHandleAt(renderBuffers[index]->GetSRVIndex());
}

ComPtr<ID3D12Resource> Window::GetCurrentScreenBuffer()
{
	int index = GetCurrentBackBufferIndex();
	return screenBuffers[index];
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentScreenRTV()
{
	DXDescriptorHeap* RTVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	int index = GetCurrentBackBufferIndex();
	return RTVHeap->GetCPUHandleAt(screenBufferRTVs[index]);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE Window::GetDepthDSV()
{
	DXDescriptorHeap* DSVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	return DSVHeap->GetCPUHandleAt(depthDSVIndex);
}

HWND Window::GetHWND() { return windowHandle; }
unsigned int Window::GetWindowWidth() { return windowWidth; }
unsigned int Window::GetWindowHeight() { return windowHeight; }
const D3D12_VIEWPORT& Window::GetViewport() { return viewport; }
const D3D12_RECT& Window::GetScissorRect() { return scissorRect; }
#pragma endregion