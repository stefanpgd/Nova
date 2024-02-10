#pragma once

#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class Window;

/// <summary>
/// The purpose of the RenderStage is to be able to encapsulate
/// render pass functionality. Whenever you need to do something
/// relating to screen-spaced pass, we go to ScreenStage.cpp
/// This also allows us to easily order our render passes in the Renderer.cpp
/// </summary>
class RenderStage
{
public:
	RenderStage(Window* window);

	virtual void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) = 0;

protected:
	Window* window;
};