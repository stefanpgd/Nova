#pragma once
#include "Window.h"
#include <string>

#include <wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <dxgi1_6.h>


class Renderer
{
public:
	Renderer(std::wstring windowName);
	~Renderer();

	void Render();
	void Resize();

private:
	void EnableDebugLayer();

	uint64_t Signal(uint64_t& fenceValue);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush(uint64_t& fenceValue);

	void CreateDevice();
	void CreateCommandQueue();
	void CreateCommandList();
	void CreateCommandAllocators();
	void CreateSynchronizationObjects();

private:
	Window* window;

	ComPtr<ID3D12Device2> device;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12CommandAllocator> commandAllocators[Window::BackBufferCount];

	// CPU-GPU Synchronization //
	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	uint64_t fenceValue = 0;
	uint64_t frameFenceValues[Window::BackBufferCount] = {};
};