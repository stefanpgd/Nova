#pragma once

#include "Window.h"

#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include <cstdint>
#include <queue>

class DXCommands
{
public:
	DXCommands();
	~DXCommands();

	void ResetCommandList(int currentBackBufferIndex);
	void ExecuteCommandList(int currentBackBufferIndex);

	void Signal();
	void Flush();
	void WaitForFenceValue(unsigned int currentBackBuffer);

	ComPtr<ID3D12CommandQueue> GetCommandQueue();
	ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

private:
	void CreateCommandQueue();
	void CreateCommandList();
	void CreateCommandAllocators();
	void CreateSynchronizationObjects();

private:
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12GraphicsCommandList2> commandList;
	ComPtr<ID3D12CommandAllocator> commandAllocators[Window::BackBufferCount];

	// CPU-GPU Synchronization //
	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	uint64_t fenceValue = 0;
	uint64_t frameFenceValues[Window::BackBufferCount] = {};
};