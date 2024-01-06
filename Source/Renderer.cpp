#include "Renderer.h"

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

#include "DXAccess.h"
#include "DXDevice.h"
#include "DXCommands.h"
#include "DXUtilities.h"
#include "Window.h"

namespace RendererInternal
{
	Window* window = nullptr;
	DXDevice* device = nullptr;
	DXCommands* commands = nullptr;

	//DXDescriptorHeap* CSUHeap = nullptr;
	//DXDescriptorHeap* DSVHeap = nullptr;
}
using namespace RendererInternal;

Renderer::Renderer(const std::wstring& applicationName)
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

void Renderer::Render()
{
}

ComPtr<ID3D12Device2> DXAccess::GetDevice()
{
	return device->Get();
}

DXCommands* DXAccess::GetCommands()
{
	return commands;
}

unsigned int DXAccess::GetCurrentBackBufferIndex()
{
	window->
}