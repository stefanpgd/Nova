#include "Framework/Scene.h"

#include "Graphics/Camera.h"
#include "Graphics/Model.h"
#include "Graphics/DXDescriptorHeap.h"
#include "Graphics/DXUtilities.h"

Scene::Scene(unsigned int windowWidth, unsigned int windowHeight)
{
	camera = new Camera(windowWidth, windowHeight);

	// Makes sure that lights get updated 
	// AFTER the renderer has fully initialized
	lightsEdited = true;
}

void Scene::Update(float deltaTime)
{
	sceneRuntime += deltaTime;

	camera->Update(deltaTime);

	// In case the lights have been edited, update lightBuffer
	if(lightsEdited)
	{
		UpdateLightBuffer();
		lightsEdited = false;
	}
}

void Scene::AddModel(const std::string& filePath)
{
	models.push_back(new Model(filePath));
}

Camera& Scene::GetCamera()
{
	return *camera;
}

const std::vector<Model*>& Scene::GetModels()
{
	return models;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE Scene::GetLightBufferHandle()
{
	DXDescriptorHeap* CBVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	return CBVHeap->GetGPUHandleAt(lightCBVIndex);
}

void Scene::UpdateLightBuffer()
{
	if(lightCBVIndex < 0)
	{
		DXDescriptorHeap* CBVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		lightCBVIndex = CBVHeap->GetNextAvailableIndex();
	}

	UpdateInFlightCBV(lightBuffer, lightCBVIndex, 1, sizeof(LightData), &lights);
}
