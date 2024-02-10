#pragma once

#include <vector>
#include <string>
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include "Graphics/Lights.h"
#include "Graphics/Camera.h"

class Model;

class Scene
{
public:
	Scene(unsigned int windowWidth, unsigned int windowHeight);

	void Update(float deltaTime);

	void AddModel(const std::string& filePath);

	Camera& GetCamera();
	const std::vector<Model*>& GetModels();
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetLightBufferHandle();

private:
	void UpdateLightBuffer();

private:
	Camera* camera;
	std::vector<Model*> models;

	// Light info //
	LightData lights;
	ComPtr<ID3D12Resource> lightBuffer;
	int lightCBVIndex = -1;
	bool lightsEdited = false;

	float sceneRuntime = 0.0f;

	friend class Editor;
};