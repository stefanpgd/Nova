#pragma once

#include <wrl.h>
using namespace Microsoft::WRL;

#include <string>
#include <d3d12.h>

class Texture
{
public:
	Texture(unsigned char*, int width, int height, int c);

	UINT GetDescriptorIndex();

private:
	ComPtr<ID3D12Resource> texture;
	UINT textureSRVIndex;

	unsigned char* imageBuffer;
	int width;
	int height;
	int channels;
};