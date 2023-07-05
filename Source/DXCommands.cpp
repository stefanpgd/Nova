#include "DXCommands.h"
#include "DXUtilities.h"
#include "DXAccess.h"

#include <cassert>
#include <chrono>

DXCommands::DXCommands()
{
	CreateCommandQueue();
	CreateCommandAllocators();
	CreateCommandList();
	CreateSynchronizationObjects();
}

DXCommands::~DXCommands()
{
	Flush(0); // placeholder //
	CloseHandle(fenceEvent);
}

void DXCommands::ResetCommandList(int currentBackBufferIndex)
{
	commandAllocators[currentBackBufferIndex]->Reset();
	commandList->Reset(commandAllocators[currentBackBufferIndex].Get(), nullptr);
}

void DXCommands::ExecuteCommandList(int currentBackBufferIndex)
{
	ThrowIfFailed(commandList->Close());

	ID3D12CommandList* const commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	Signal();
	frameFenceValues[currentBackBufferIndex] = fenceValue;
}

void DXCommands::Signal()
{
	fenceValue++;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));
}

void DXCommands::WaitForFenceValue(unsigned int currentBackBuffer)
{
	if (fence->GetCompletedValue() < frameFenceValues[currentBackBuffer])
	{
		ThrowIfFailed(fence->SetEventOnCompletion(frameFenceValues[currentBackBuffer], fenceEvent));
		WaitForSingleObject(fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
	}
}

// We signal first, so we've a new fence value to look out for, then right after we 
// wait until we've reached that fence value. Because this is the newest fence value, all other
// command lists that were running must've been finished before the new fence value has been reached.
void DXCommands::Flush(int currentBackBufferIndex)
{
	Signal();

	for (int i = 0; i < Window::BackBufferCount; i++)
	{
		WaitForFenceValue(i);
	}
}

ComPtr<ID3D12CommandQueue> DXCommands::GetCommandQueue()
{
	return commandQueue;
}

ComPtr<ID3D12GraphicsCommandList2> DXCommands::GetCommandList()
{
	return commandList;
}

void DXCommands::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC description = {};
	description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	description.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	description.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	description.NodeMask = 0;

	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	ThrowIfFailed(device->CreateCommandQueue(&description, IID_PPV_ARGS(&commandQueue)));
}

void DXCommands::CreateCommandList()
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&commandList)));

	// Command List open in a recording state, since in our render loop we start with `Reset`
	// we want to close the command list first so it can be properly reset.
	ThrowIfFailed(commandList->Close());
}

void DXCommands::CreateCommandAllocators()
{
	// Command Allocator //
	for (int i = 0; i < Window::BackBufferCount; ++i)
	{
		ComPtr<ID3D12CommandAllocator> commandAllocator;

		ComPtr<ID3D12Device2> device = DXAccess::GetDevice();
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
		commandAllocators[i] = commandAllocator;
	}
}

void DXCommands::CreateSynchronizationObjects()
{
	ComPtr<ID3D12Device2> device = DXAccess::GetDevice();

	// Fence //
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// Event Handle //
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create");
}