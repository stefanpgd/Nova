#pragma once

#include <vector>
#include <string>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include "DXUtilities.h"
#include "tiny_gltf.h"

struct Vertex
{
	float3 Position;
	float3 Normal;
	float3 Color;
};

class Mesh
{
public:
	Mesh(tinygltf::Model& model, tinygltf::Primitive& primitive, matrix& transform);

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView();
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView();
	const unsigned int GetIndicesCount();

private:
	void LoadAttribute(tinygltf::Model& model, tinygltf::Primitive& primitive, const std::string& attributeType);
	void LoadIndices(tinygltf::Model& model, tinygltf::Primitive& primitive);
	void LoadMaterial(tinygltf::Model& model, tinygltf::Primitive& primitive);

	void ApplyNodeTransform(matrix& transform);
	void UploadBuffers();


private:
	matrix placeholderM;

	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	unsigned int indicesCount = 0;
};