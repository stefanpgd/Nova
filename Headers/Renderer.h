#pragma once
#include <string>

// TODO: For now window size stuff is handled in Renderer
// in the future move it with serialization to Engine.

// TODO: Fullscreen support
// TODO: Screen tearing support

class DXRootSignature;
class DXPipeline;

class Renderer
{
public:
	Renderer(const std::wstring& applicationName);

	void Render();

private:
	void InitializeImGui();

	DXRootSignature* rootSignature;
	DXPipeline* pipeline;

	unsigned int frameCount = 0;
	unsigned int width = 1080;
	unsigned int height = 720;
};