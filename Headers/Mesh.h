#pragma once

#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include "DXUtilities.h"

struct Vertex
{
	float3 Position;
	float3 Color;
};

class Mesh
{
public:
	Mesh();

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView();
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView();
	const unsigned int GetIndicesCount();

private:
	void UploadBuffers();

private:
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	unsigned int indicesCount = 0;
};