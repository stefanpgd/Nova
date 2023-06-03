#include "Renderer.h"
#include "Utilities.h"

#include <d3d12.h>
#include <cassert>
#include <chrono>

Renderer::Renderer(std::wstring windowName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	EnableDebugLayer();

	CreateDevice();
	CreateCommandQueue();

	window = new Window(windowName, 1280, 720, device, commandQueue);

	CreateCommandAllocators();
	CreateCommandList();
	CreateSynchronizationObjects();
}

Renderer::~Renderer()
{
	Flush(fenceValue);
	CloseHandle(fenceEvent);
}

void Renderer::Render()
{
	unsigned int currentBufferIndex = window->GetCurrentBackBufferIndex();
	ComPtr<ID3D12CommandAllocator> commandAllocator = commandAllocators[currentBufferIndex];
	ComPtr<ID3D12Resource> backBuffer = window->GetCurrentBackBuffer();

	commandAllocator->Reset();
	commandList->Reset(commandAllocator.Get(), nullptr);

	CD3DX12_RESOURCE_BARRIER renderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &renderBarrier);

	float clearColor[4] = { 0.2f, 0.45f, 0.31f, 1.0f };
	commandList->ClearRenderTargetView(window->GetCurrentBackBufferRTV(), clearColor, 0, nullptr);

	CD3DX12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &presentBarrier);

	ThrowIfFailed(commandList->Close());

	ID3D12CommandList* const commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	frameFenceValues[currentBufferIndex] = Signal(fenceValue);

	window->Present();

	WaitForFenceValue(frameFenceValues[currentBufferIndex]);
}

void Renderer::Resize()
{
	Flush(fenceValue);
	window->Resize();
}

void Renderer::EnableDebugLayer()
{
#if defined (_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

uint64_t Renderer::Signal(uint64_t& fenceValue)
{
	uint64_t signalValue = ++fenceValue;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), signalValue));
	return signalValue;
}

void Renderer::WaitForFenceValue(uint64_t fenceValue)
{
	if (fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
	}
}

void Renderer::Flush(uint64_t& fenceValue)
{
	uint64_t fenceValueForSignal = Signal(fenceValue);
	WaitForFenceValue(fenceValueForSignal);
}

void Renderer::CreateDevice()
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	// Enables errors to be caught during device creation and while quering adapaters
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	SIZE_T maxDedicatedVideoMemory = 0;
	for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

		// Is adapter not a software GPU? Can it succesfully create a Dx12 device?
		// Does it have more dedicated video memory than the other adapters? If so, store it as the most capable adapter
		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
			SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
				D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
			dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
		}
	}

	ThrowIfFailed(D3D12CreateDevice(dxgiAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));

#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(device.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,  
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                        
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                      
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif
}

void Renderer::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC description = {};
	description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	description.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	description.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	description.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&description, IID_PPV_ARGS(&commandQueue)));
}

void Renderer::CreateCommandList()
{
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocators[window->GetCurrentBackBufferIndex()].Get(), nullptr, IID_PPV_ARGS(&commandList)));

	// Command List open in a recording state, since in our render loop we start with `Reset`
	// we want to close the command list first so it can be properly reset.
	ThrowIfFailed(commandList->Close());
}

void Renderer::CreateCommandAllocators()
{
	// Command Allocator //
	for (int i = 0; i < Window::BackBufferCount; ++i)
	{
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

		commandAllocators[i] = commandAllocator;
	}
}

void Renderer::CreateSynchronizationObjects()
{
	// Fence //
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// Event Handle //
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create");
}