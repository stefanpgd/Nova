#pragma once
#include "Window.h"
#include <string>

#include <wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <dxgi1_6.h>

#include "DXCommandQueue.h"

class Renderer
{
public:
	Renderer(std::wstring windowName);
	~Renderer();

	void Render();
	void Resize();

private:
	void EnableDebugLayer();



	void CreateDevice();

private:
	Window* window;
	DXCommandQueue* commandQueue;
	ComPtr<ID3D12Device2> device;
};