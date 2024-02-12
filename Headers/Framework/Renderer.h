#pragma once
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include "Graphics/Lights.h"

class Scene;

class ShadowStage;
class SceneStage;
class ScreenStage;
class SkydomeStage;

class Renderer
{
public:
	Renderer(const std::wstring& applicationName, Scene* scene, unsigned int windowWidth, unsigned int windowHeight);

	void Update(float deltaTime);
	void Render();

	void SetScene(Scene* newScene);
	void Resize();

private:
	void InitializeImGui();

private:
	// Rendering Stages //
	ShadowStage* shadowStage;
	SceneStage* sceneStage;
	ScreenStage* screenStage;
	SkydomeStage* skydomeStage;

	Scene* scene;
};