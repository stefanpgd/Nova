#include "Graphics/Mesh.h"
#include "Graphics/DXAccess.h"
#include "Graphics/DXUtilities.h"
#include "Graphics/Texture.h"
#include <cassert>

Mesh::Mesh(tinygltf::Model& model, tinygltf::Primitive& primitive, glm::mat4& transform)
{
	// A 'Mesh' exists out of multiple primitives, usually this is one
	// but it can be more. Each primitive contains the geometry data (triangles, lines etc. )
	// to render the model
	LoadAttribute(model, primitive, "POSITION");
	LoadAttribute(model, primitive, "NORMAL");
	LoadAttribute(model, primitive, "TEXCOORD_0");

	LoadMaterial(model, primitive);
	LoadIndices(model, primitive);

	ApplyNodeTransform(transform);
	UploadBuffers();
}

Mesh::Mesh(Vertex* verts, unsigned int vertexCount, unsigned int* indi, unsigned int indexCount)
{
	for(int i = 0; i < vertexCount; i++)
	{
		vertices.push_back(verts[i]);
	}

	for(int i = 0; i < indexCount; i++)
	{
		indices.push_back(indi[i]);
	}

	UploadBuffers();
}

const D3D12_VERTEX_BUFFER_VIEW& Mesh::GetVertexBufferView()
{
	return vertexBufferView;
}

const D3D12_INDEX_BUFFER_VIEW& Mesh::GetIndexBufferView()
{
	return indexBufferView;
}

const unsigned int Mesh::GetIndicesCount()
{
	return indicesCount;
}

bool Mesh::HasTextures()
{
	return loadedTextures;
}

// TODO: add type of texture in here to get ID
unsigned int Mesh::GetTextureID()
{
	return albedoTexture->GetSRVIndex();
}

void Mesh::LoadAttribute(tinygltf::Model& model, tinygltf::Primitive& primitive, const std::string& attributeType)
{
	auto attribute = primitive.attributes.find(attributeType);

	// Check if within the primitives's attributes the type is present. For example 'Normals'
	// If not, stop here, the model isn't valid
	if (attribute == primitive.attributes.end())
	{
		std::string message = "Attribute Type: '" + attributeType + "' missing from model.";
		LOG(Log::MessageType::Debug, message);
		return;
	}

	// Accessor: Tells use which view we need, what type of data is in it, and the amount/count of data.
	// BufferView: Tells which buffer we need, and where we need to be in the buffer
	// Buffer: Binary data of our mesh
	tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at(attributeType)];
	tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
	tinygltf::Buffer& buffer = model.buffers[view.buffer];

	// Component: default type like float, int
	// Type: a structure made out of components, e.g VEC2 ( 2x float )
	unsigned int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	unsigned int objectSize = tinygltf::GetNumComponentsInType(accessor.type);
	unsigned int dataSize = componentSize * objectSize;

	// Accessor byteoffset: Offset to first element of type
	// BufferView byteoffset: Offset to get to this primitives buffer data in the overall buffer
	unsigned int bufferStart = accessor.byteOffset + view.byteOffset;

	// Stride: Distance in buffer till next elelemt occurs
	unsigned int stride = accessor.ByteStride(view);

	// In case it hasn't happened, resize the vertex buffer since we're 
	// going to directly memcpy the data into an already existing buffer
	if (vertices.size() < accessor.count)
	{
		vertices.resize(accessor.count);
	}

	for (int i = 0; i < accessor.count; i++)
	{
		Vertex& vertex = vertices[i];
		size_t bufferLocation = bufferStart + (i * stride);

		// TODO: Add 'offsetto' to this and use pointers to vertices instead of a reference
		if (attributeType == "POSITION")
		{
			memcpy(&vertex.Position, &buffer.data[bufferLocation], dataSize);
		}
		else if (attributeType == "NORMAL")
		{
			memcpy(&vertex.Normal, &buffer.data[bufferLocation], dataSize);
		}
		else if(attributeType == "TEXCOORD_0")
		{
			memcpy(&vertex.TexCoord, &buffer.data[bufferLocation], dataSize);
		}
	}
}

void Mesh::LoadIndices(tinygltf::Model& model, tinygltf::Primitive& primitive)
{
	tinygltf::Accessor& accessor = model.accessors[primitive.indices];
	tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
	tinygltf::Buffer& buffer = model.buffers[view.buffer];

	unsigned int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	unsigned int objectSize = tinygltf::GetNumComponentsInType(accessor.type);
	unsigned int dataSize = componentSize * objectSize;

	unsigned int bufferStart = accessor.byteOffset + view.byteOffset;
	unsigned int stride = accessor.ByteStride(view);

	for (int i = 0; i < accessor.count; i++)
	{
		size_t bufferLocation = bufferStart + (i * stride);

		if (componentSize == 2)
		{
			short index;
			memcpy(&index, &buffer.data[bufferLocation], dataSize);
			indices.push_back(index);
		}
		else if (componentSize == 4)
		{
			unsigned int index;
			memcpy(&index, &buffer.data[bufferLocation], dataSize);
			indices.push_back(index);
		}
	}
}

void Mesh::LoadMaterial(tinygltf::Model& model, tinygltf::Primitive& primitive)
{
	// For now, do only color //
	tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];
	tinygltf::Material& material = model.materials[primitive.material];

	for (int i = 0; i < accessor.count; i++)
	{
		Vertex& vertex = vertices[i];

		// TODO: Memcpy or send pointer
		vertex.Color.x = material.pbrMetallicRoughness.baseColorFactor[0];
		vertex.Color.y = material.pbrMetallicRoughness.baseColorFactor[1];
		vertex.Color.z = material.pbrMetallicRoughness.baseColorFactor[2];
	}

	int textureID = material.pbrMetallicRoughness.baseColorTexture.index;
	if (textureID == -1)
	{
		LOG(Log::MessageType::Debug, "Texture 'BaseColor' is not available");
		return;
	}

	albedoTexture = new Texture(model, model.textures[textureID]);
	loadedTextures = true;
}

void Mesh::ApplyNodeTransform(const glm::mat4& transform)
{
	// TODO: Check if it can be optimized or something with GLM, or if vec3s work
	for (Vertex& vertex : vertices)
	{
		glm::vec4 vert = glm::vec4(vertex.Position.x, vertex.Position.y, vertex.Position.z, 1.0f);
		vertex.Position = transform * vert;

		glm::vec4 norm = glm::vec4(vertex.Normal.x, vertex.Normal.y, vertex.Normal.z, 0.0f);
		vertex.Normal = glm::normalize(transform * norm);
	}
}

void Mesh::UploadBuffers()
{
	DXCommands* copyCommands = DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_COPY);
	ComPtr<ID3D12GraphicsCommandList2> copyCommandList = copyCommands->GetGraphicsCommandList();
	copyCommands->ResetCommandList();

	// 1. Record commands to upload vertex & index buffers //
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	UpdateBufferResource(copyCommandList, &vertexBuffer, &intermediateVertexBuffer, vertices.size(),
		sizeof(Vertex), vertices.data(), D3D12_RESOURCE_FLAG_NONE);

	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(copyCommandList, &indexBuffer, &intermediateIndexBuffer, indices.size(),
		sizeof(unsigned int), indices.data(), D3D12_RESOURCE_FLAG_NONE);

	// 2. Retrieve info about from the buffers to create Views  // 
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = vertices.size() * sizeof(Vertex);
	vertexBufferView.StrideInBytes = sizeof(Vertex);

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = indices.size() * sizeof(unsigned int);
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// 3.Execute the copying on the command queue & wait until it's done // 
	copyCommands->ExecuteCommandList(DXAccess::GetCurrentBackBufferIndex());
	copyCommands->Signal();
	copyCommands->WaitForFenceValue(DXAccess::GetCurrentBackBufferIndex());

	// 4. Clear CPU data // 
	indicesCount = indices.size();
	vertices.clear();
	indices.clear();
}