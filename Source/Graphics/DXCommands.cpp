#include "Graphics/DXCommands.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXDevice.h"
#include "Graphics/DXUtilities.h"

#include <cassert>
#include <chrono>

DXCommands::DXCommands(D3D12_COMMAND_LIST_TYPE type, unsigned int commandAllocatorCount) : commandAllocatorCount(commandAllocatorCount)
{
	if(commandAllocatorCount == 0)
	{
		assert(false && "Must have at least 1 command allocator");
	}

	frameFenceValues = new uint64_t[commandAllocatorCount];

	for(int i = 0; i < commandAllocatorCount; i++)
	{
		frameFenceValues[i] = 0;
	}

	device = DXAccess::GetDevice();

	CreateCommandQueue(type);
	CreateCommandAllocators();
	CreateCommandList();
	CreateSynchronizationObjects();
}

DXCommands::~DXCommands()
{
	Flush();
	CloseHandle(fenceEvent);
}

void DXCommands::ExecuteCommandList(int allocatorIndex)
{
	ThrowIfFailed(commandList->Close());

	ID3D12CommandList* const commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	Signal();
	frameFenceValues[allocatorIndex] = fenceValue;
}

void DXCommands::ResetCommandList(int allocatorIndex)
{
	commandAllocators[allocatorIndex]->Reset();
	commandList->Reset(commandAllocators[allocatorIndex].Get(), nullptr);
}

void DXCommands::Signal()
{
	fenceValue++;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValue));
}

void DXCommands::Flush()
{
	Signal();

	if(fence->GetCompletedValue() < fenceValue)
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		WaitForSingleObject(fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
	}
}

void DXCommands::WaitForFenceValue(unsigned int allocatorIndex)
{
	if(fence->GetCompletedValue() < frameFenceValues[allocatorIndex])
	{
		ThrowIfFailed(fence->SetEventOnCompletion(frameFenceValues[allocatorIndex], fenceEvent));
		WaitForSingleObject(fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
	}
}

ComPtr<ID3D12CommandQueue> DXCommands::GetCommandQueue()
{
	return commandQueue;
}

ComPtr<ID3D12CommandList> DXCommands::GetCommandList()
{
	ComPtr<ID3D12CommandList> commands;
	commandList.As(&commands);

	return commands;
}

ComPtr<ID3D12GraphicsCommandList2> DXCommands::GetGraphicsCommandList()
{
	return commandList;
}

void DXCommands::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC description = {};
	description.Type = type;
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
	for(int i = 0; i < commandAllocatorCount; i++)
	{
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

		commandAllocators[i] = commandAllocator;
	}
}

void DXCommands::CreateSynchronizationObjects()
{
	// Fence //
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	// Fence Event //
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create Fence Event");
}