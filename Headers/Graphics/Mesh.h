#pragma once

#include <vector>
#include <string>
#include <d3d12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include "Framework/Mathematics.h"
#include "tiny_gltf.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 Color;
	glm::vec2 TexCoord;
};

class Texture;

class Mesh
{
public:
	Mesh(tinygltf::Model& model, tinygltf::Primitive& primitive, glm::mat4& transform);

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView();
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView();
	const unsigned int GetIndicesCount();

	bool HasTextures();
	unsigned int GetTextureID();

private:
	void LoadAttribute(tinygltf::Model& model, tinygltf::Primitive& primitive, const std::string& attributeType);
	void LoadIndices(tinygltf::Model& model, tinygltf::Primitive& primitive);
	void LoadMaterial(tinygltf::Model& model, tinygltf::Primitive& primitive);

	void ApplyNodeTransform(const glm::mat4& transform);
	void UploadBuffers();

private:
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	unsigned int indicesCount = 0;

	bool loadedTextures = false;
	Texture* albedoTexture;
};