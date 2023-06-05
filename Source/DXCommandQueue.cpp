#include "DXCommandQueue.h"
#include "Utilities.h"

#include <cassert>
#include <chrono>

DXCommandQueue::DXCommandQueue(ComPtr<ID3D12Device2> device) : device(device)
{
	CreateCommandQueue();
	CreateCommandAllocators();
	CreateCommandList();
	CreateSynchronizationObjects();
}

DXCommandQueue::~DXCommandQueue()
{
	Flush();
	CloseHandle(fenceEvent);
}

void DXCommandQueue::ResetCommandList(int currentBackBufferIndex)
{
	commandAllocators[currentBackBufferIndex]->Reset();
	commandList->Reset(commandAllocators[currentBackBufferIndex].Get(), nullptr);
}

void DXCommandQueue::ExecuteCommandList(int currentBackBufferIndex)
{
	ThrowIfFailed(commandList->Close());

	ID3D12CommandList* const commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	Signal();
	frameFenceValues[currentBackBufferIndex] = fenceValue;
}

void DXCommandQueue::Signal()
{
	fenceValue++;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));
}

void DXCommandQueue::WaitForFenceValue(uint64_t targetFenceValue)
{
	if (fence->GetCompletedValue() < targetFenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(targetFenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
	}
}

// We signal first, so we've a new fence value to look out for, then right after we 
// wait until we've reached that fence value. Because this is the newest fence value, all other
// command lists that were running must've been finished before the new fence value has been reached.
void DXCommandQueue::Flush()
{
	Signal();
	WaitForFenceValue(fenceValue);
}

ComPtr<ID3D12CommandQueue> DXCommandQueue::Get()
{
	return commandQueue;
}

ComPtr<ID3D12GraphicsCommandList2> DXCommandQueue::GetCommandList()
{
	return commandList;
}

void DXCommandQueue::CreateCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC description = {};
	description.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	description.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	description.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	description.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&description, IID_PPV_ARGS(&commandQueue)));
}

void DXCommandQueue::CreateCommandList()
{
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocators[0].Get(), nullptr, IID_PPV_ARGS(&commandList)));

	// Command List open in a recording state, since in our render loop we start with `Reset`
	// we want to close the command list first so it can be properly reset.
	ThrowIfFailed(commandList->Close());
}

void DXCommandQueue::CreateCommandAllocators()
{
	// Command Allocator //
	for (int i = 0; i < Window::BackBufferCount; ++i)
	{
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
		commandAllocators[i] = commandAllocator;
	}
}

void DXCommandQueue::CreateSynchronizationObjects()
{
	// Fence //
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// Event Handle //
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create");
}