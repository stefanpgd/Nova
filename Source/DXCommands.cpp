#include "DXCommands.h"

DXCommands::DXCommands()
{
}

DXCommands::~DXCommands()
{
}

void DXCommands::ExecuteCommandList(int currentBackBufferIndex)
{
}

void DXCommands::RestCommandList(int currentBackBufferIndex)
{
}

void DXCommands::Signal()
{
}

void DXCommands::Flush()
{
}

void DXCommands::WaitForFenceValue(unsigned int backBufferIndex)
{
}

ComPtr<ID3D12CommandQueue> DXCommands::GetCommandQueue()
{
	return commandQueue;
}

ComPtr<ID3D12CommandList> DXCommands::GetCommandList()
{
	return commandList;
}

void DXCommands::CreateCommandQueue()
{
}

void DXCommands::CreateCommandList()
{
}

void DXCommands::CreateCommandAllocators()
{
}

void DXCommands::CreateSynchronizationObjects()
{
}