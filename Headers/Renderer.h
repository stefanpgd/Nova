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

	void Update(float deltaTime);
	void Render();

	void AddModel(const std::string& filePath);

private:
	void InitializeImGui();

	DXRootSignature* rootSignature;
	DXPipeline* pipeline;

	float elaspedTime = 0.0f;

	unsigned int width = 1080;
	unsigned int height = 720;
};