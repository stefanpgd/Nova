#pragma once

#include <vector>
#include <string>
#include <d3d12.h>
#include <d3dx12.h>
#include <wrl.h>
using namespace Microsoft::WRL;

#include "Framework/Mathematics.h"
#include "tiny_gltf.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec3 Color;
	glm::vec2 TexCoord;
};

struct Material
{
	bool hasAlbedo;
	bool hasNormal;
	bool hasMetallicRoughness;
	bool hasOclussion;

	unsigned int oChannel = 0;
	unsigned int rChannel = 1;
	unsigned int mChannel = 2;

	// GPU Memory alignment //
	double stub[30];
};

class Texture;

class Mesh
{
public:
	Mesh(tinygltf::Model& model, tinygltf::Primitive& primitive, glm::mat4& transform);
	Mesh(Vertex* vertices, unsigned int vertexCount, unsigned int* indices, unsigned int indexCount);

	void UpdateMaterialData();

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView();
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView();
	const CD3DX12_GPU_DESCRIPTOR_HANDLE GetMaterialView();
	const unsigned int GetIndicesCount();

	bool HasTextures();
	unsigned int GetTextureID();

private:
	void LoadAttribute(tinygltf::Model& model, tinygltf::Primitive& primitive, const std::string& attributeType);
	void LoadIndices(tinygltf::Model& model, tinygltf::Primitive& primitive);
	void LoadMaterial(tinygltf::Model& model, tinygltf::Primitive& primitive);

	void GenerateTangents();

	void ApplyNodeTransform(const glm::mat4& transform);
	void UploadBuffers();

private:
	// Vertex & Index Data //
	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	unsigned int indicesCount = 0;

	// Texture & Material Data //
	bool loadedTextures = false;

	Texture* albedoTexture = nullptr;
	Texture* normalTexture = nullptr;
	Texture* metallicRoughnessTexture = nullptr;
	Texture* ambientOcclusionTexture = nullptr;

	int materialCBVIndex = -1;
	Material material;
	ComPtr<ID3D12Resource> materialBuffer;
};