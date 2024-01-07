#include "DXCommands.h"
#include "DXAccess.h"
#include "DXDevice.h"
#include "DXUtilities.h"

#include <cassert>
#include <chrono>

DXCommands::DXCommands()
{
	device = DXAccess::GetDevice();

	CreateCommandQueue();
	CreateCommandAllocators();
	CreateCommandList();
	CreateSynchronizationObjects();
}

DXCommands::~DXCommands()
{
	Flush();
	CloseHandle(fenceEvent);
}

void DXCommands::ExecuteCommandList(int backBufferIndex)
{
	ThrowIfFailed(commandList->Close());

	ID3D12CommandList* const commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	Signal();
	frameFenceValues[backBufferIndex] = fenceValue;
}

void DXCommands::ResetCommandList(int backBufferIndex)
{
	commandAllocators[backBufferIndex]->Reset();
	commandList->Reset(commandAllocators[backBufferIndex].Get(), nullptr);
}

void DXCommands::Signal()
{
	fenceValue++;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));
}

/// <summary>
/// Forces the CPU to stall until every executed command list has finished executing
/// </summary>
void DXCommands::Flush()
{
	Signal();

	for(int i = 0; i < Window::BackBufferCount; i++)
	{
		WaitForFenceValue(i);
	}
}

void DXCommands::WaitForFenceValue(unsigned int backBufferIndex)
{
	if(fence->GetCompletedValue() < frameFenceValues[backBufferIndex])
	{
		ThrowIfFailed(fence->SetEventOnCompletion(frameFenceValues[backBufferIndex], fenceEvent));
		WaitForSingleObject(fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
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

	ThrowIfFailed(device->CreateCommandQueue(&description, IID_PPV_ARGS(&commandQueue)));
}

void DXCommands::CreateCommandList()
{
	ComPtr<ID3D12CommandAllocator> commandAllocator = commandAllocators[0];
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	// Command List open in a recording state, since in our render loop we start with `Reset`
	// we want to close the command list first so it can be properly reset.
	ThrowIfFailed(commandList->Close());
}

void DXCommands::CreateCommandAllocators()
{
	for(int i = 0; i < Window::BackBufferCount; i++)
	{
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

		commandAllocators[i] = commandAllocator;
	}
}

void DXCommands::CreateSynchronizationObjects()
{
	// Fence
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// Fence Event
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create Fence Event");
}