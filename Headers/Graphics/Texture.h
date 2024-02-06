#pragma once

#include <tiny_gltf.h>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

class Texture
{
public:
	Texture(tinygltf::Model& model, tinygltf::Texture& texture);

	int GetSRVIndex();

private:
	ComPtr<ID3D12Resource> textureResource;
	int srvIndex = 0;
};