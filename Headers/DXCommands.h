#pragma once
#include "Window.h"

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <cstdint>

// TODO: Update the Commands to support Upload Version. means backbuffer stuff needs to be optional?
class DXCommands
{
public:
	DXCommands();
	~DXCommands();

	void ExecuteCommandList(int backBufferIndex);
	void ResetCommandList(int backBufferIndex);

	void Signal();
	void Flush();
	void WaitForFenceValue(unsigned int backBufferIndex);

	ComPtr<ID3D12CommandQueue> GetCommandQueue();
	ComPtr<ID3D12GraphicsCommandList2> GetCommandList();

private:
	void CreateCommandQueue();
	void CreateCommandList();
	void CreateCommandAllocators();
	void CreateSynchronizationObjects();

private:
	ComPtr<ID3D12Device2> device;

	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12GraphicsCommandList2> commandList;
	ComPtr<ID3D12CommandAllocator> commandAllocators[Window::BackBufferCount];

	// CPU-GPU Synchronization //
	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	uint64_t frameFenceValues[Window::BackBufferCount] = {};
	uint64_t fenceValue = 0;
};