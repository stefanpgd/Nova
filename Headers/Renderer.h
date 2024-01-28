#pragma once
#include <string>
#include <vector>

// TODO: For now window size stuff is handled in Renderer
// in the future move it with serialization to Engine.

// TODO: Fullscreen support
// TODO: Screen tearing support

class DXRootSignature;
class DXPipeline;

class Camera;
class Model;

class Renderer
{
public:
	Renderer(const std::wstring& applicationName);

	void Update(float deltaTime);
	void Render();

	void Resize();
	void AddModel(const std::string& filePath);

private:
	void InitializeImGui();

private:
	DXRootSignature* rootSignature;
	DXPipeline* pipeline;
	Camera* camera; 

	std::vector<Model*> models;

	float elaspedTime = 0.0f;
	unsigned int width = 1080;
	unsigned int height = 720;
};