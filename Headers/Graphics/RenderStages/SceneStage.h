#pragma once
#include "Graphics/RenderStage.h"

#include <vector>

class Model;
struct LightData;
class Camera;

// TODO: replace pointers with something more safe 
class SceneStage : public RenderStage
{
public:
	SceneStage(Window* window, Camera* camera, 
		std::vector<Model*>& models, LightData* lights, int lightCBVIndex);

	void RecordStage(ComPtr<ID3D12GraphicsCommandList2> commandList) override;

private:
	void CreatePipeline();

private:
	std::vector<Model*>& models;
	LightData* lights;
	Camera* camera;

	const int lightDataCBVIndex;
};