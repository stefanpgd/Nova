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
	LoadAttribute(model, primitive, "TANGENT");
	LoadAttribute(model, primitive, "TEXCOORD_0");

	LoadIndices(model, primitive);
	LoadMaterial(model, primitive);

	GenerateTangents();
	
	ApplyNodeTransform(transform);

	UploadBuffers();
	UpdateMaterialData();
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
	UpdateMaterialData();
}

void Mesh::UpdateMaterialData()
{
	if(materialCBVIndex < 0)
	{
		DXDescriptorHeap* CBVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		materialCBVIndex = CBVHeap->GetNextAvailableIndex();
	}

	UpdateInFlightCBV(materialBuffer, materialCBVIndex, 1, sizeof(Material), &material);
}

const D3D12_VERTEX_BUFFER_VIEW& Mesh::GetVertexBufferView()
{
	return vertexBufferView;
}

const D3D12_INDEX_BUFFER_VIEW& Mesh::GetIndexBufferView()
{
	return indexBufferView;
}

const CD3DX12_GPU_DESCRIPTOR_HANDLE Mesh::GetMaterialView()
{
	DXDescriptorHeap* CBVHeap = DXAccess::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_GPU_DESCRIPTOR_HANDLE handle = CBVHeap->GetGPUHandleAt(materialCBVIndex);
	return handle;
}

const unsigned int Mesh::GetIndicesCount()
{
	return indicesCount;
}

bool Mesh::HasTextures()
{
	return hasTextures;
}

unsigned int Mesh::GetTextureID()
{
	// Returns the first SRV of the group:
	// Albedo, Normal, MetallicRoughness, Ambient Occlusion
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
		else if(attributeType == "TANGENT")
		{
			memcpy(&vertex.Tangent, &buffer.data[bufferLocation], dataSize);
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
	tinygltf::Material& mat = model.materials[primitive.material];

	for (int i = 0; i < accessor.count; i++)
	{
		Vertex& vertex = vertices[i];

		// TODO: Memcpy or send pointer
		vertex.Color.x = mat.pbrMetallicRoughness.baseColorFactor[0];
		vertex.Color.y = mat.pbrMetallicRoughness.baseColorFactor[1];
		vertex.Color.z = mat.pbrMetallicRoughness.baseColorFactor[2];
	}

	int albedoID = mat.pbrMetallicRoughness.baseColorTexture.index;
	int normalID = mat.normalTexture.index;
	int metallicRoughnessID = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
	int occlusionID = mat.occlusionTexture.index;

	LoadTexture(model, &albedoTexture, albedoID, material.hasAlbedo);
	LoadTexture(model, &normalTexture, normalID, material.hasNormal);
	LoadTexture(model, &metallicRoughnessTexture, metallicRoughnessID, material.hasMetallicRoughness);
	LoadTexture(model, &occlusionTexture, occlusionID, material.hasOcclusion);

	// Incase a mesh is loaded through a TinyglTF primitive, it is assumed
	// either textures or colors were present, when the other Mesh constructor is used
	// with raw data, then it's assumed no textures are present. For example with the Screen Quad.
	hasTextures = true;
}

void Mesh::LoadTexture(tinygltf::Model& model, Texture** texture, int textureID, int& materialCheck)
{
	if(textureID != -1)
	{
		*texture = new Texture(model, model.textures[textureID]);
		materialCheck = 1;
	}
	else
	{
		// TODO: Incase we are allowed to bind Individual SRVs, then do this, temporarily we do this 
		//*texture = DXAccess::GetDefaultTexture();
		*texture = new Texture("Assets/Textures/error.jpg");
		materialCheck = 0;
	}
}

void Mesh::GenerateTangents()
{
	Vertex& vertex = vertices[0];

	// Incase the vertex doesn't have the default value of a zero-vector
	// it means that the Tangent attribute was present for the model
	// if not, we need to generate them.
	if(vertex.Tangent != glm::vec3(0.0f))
	{
		return;
	}

	// Grab the average tangent of all triangles in the model //
	for(unsigned int i = 0; i < indices.size(); i += 3)
	{
		Vertex& v0 = vertices[indices[i]];
		Vertex& v1 = vertices[indices[i + 1]];
		Vertex& v2 = vertices[indices[i + 2]];

		glm::vec3 tangent;

		// Edges of triangles //
		glm::vec3 edge1 = v1.Position - v0.Position;
		glm::vec3 edge2 = v2.Position - v0.Position;
		
		// UV deltas //
		glm::vec2 deltaUV1 = v1.TexCoord - v0.TexCoord;
		glm::vec2 deltaUV2 = v2.TexCoord - v0.TexCoord;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
		tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * f;

		v0.Tangent += tangent;
		v1.Tangent += tangent;
		v2.Tangent += tangent;

		v0.Tangent = glm::normalize(v0.Tangent);
		v1.Tangent = glm::normalize(v1.Tangent);
		v2.Tangent = glm::normalize(v2.Tangent);
	}
}

void Mesh::ApplyNodeTransform(const glm::mat4& transform)
{
	for (Vertex& vertex : vertices)
	{
		glm::vec4 vert = glm::vec4(vertex.Position.x, vertex.Position.y, vertex.Position.z, 1.0f);
		vertex.Position = transform * vert;

		glm::vec4 norm = glm::vec4(vertex.Normal.x, vertex.Normal.y, vertex.Normal.z, 0.0f);
		vertex.Normal = glm::normalize(transform * norm);

		glm::vec4 tang = glm::vec4(vertex.Tangent.x, vertex.Tangent.y, vertex.Tangent.z, 0.0f);
		vertex.Tangent = glm::normalize(transform * tang);
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