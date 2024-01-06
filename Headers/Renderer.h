#pragma once
#include <string>

// TODO: For now window size stuff is handled in Renderer
// in the future move it with serialization to Engine.

// TODO: Fullscreen support
// TODO: Screen tearing support

class Renderer
{
public:
	Renderer(const std::wstring& applicationName);

	void Render();
};