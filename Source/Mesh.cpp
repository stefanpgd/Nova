#include "Mesh.h"
#include "DXUtilities.h"
#include "DXAccess.h"
#include "DXCommands.h"

#include <cassert>

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int> indices)
{
	// Upload vertex buffer //
	UploadBufferToResource(&vertexBuffer, &intermediateVertexBuffer, vertices.size(),
		sizeof(Vertex), vertices.data());

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = vertices.size() * sizeof(Vertex);
	vertexBufferView.StrideInBytes = sizeof(Vertex);

	// Upload Index Buffer //
	UploadBufferToResource(&indexBuffer, &intermediateIndexBuffer, indices.size(),
		sizeof(unsigned int), indices.data());

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = sizeof(unsigned int) * indices.size();

	amountOfIndices = indices.size();
}

Mesh::Mesh(tinygltf::Model& model, tinygltf::Mesh& mesh)
{
	for (int i = 0; i < mesh.primitives.size(); i++)
	{
		tinygltf::Primitive& primitive = mesh.primitives[i];
		LoadVertices(model, primitive);
		LoadIndices(model, primitive);
	}

	// Upload vertex buffer //
	UploadBufferToResource(&vertexBuffer, &intermediateVertexBuffer, vertices.size(),
		sizeof(Vertex), vertices.data());

	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = vertices.size() * sizeof(Vertex);
	vertexBufferView.StrideInBytes = sizeof(Vertex);

	// Upload Index Buffer //
	UploadBufferToResource(&indexBuffer, &intermediateIndexBuffer, indices.size(),
		sizeof(unsigned int), indices.data());

	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = sizeof(unsigned int) * indices.size();

	amountOfIndices = indices.size();
}

void Mesh::SetAndDraw()
{
	ComPtr<ID3D12GraphicsCommandList2> commandList = DXAccess::GetCommands()->GetCommandList();
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);
	commandList->DrawIndexedInstanced(amountOfIndices, 1, 0, 0, 0);
}

void Mesh::ClearIntermediateBuffers()
{
	intermediateVertexBuffer.Reset();
	intermediateIndexBuffer.Reset();
}

void Mesh::LoadVertices(tinygltf::Model& model, tinygltf::Primitive& primitive)
{
	// Verify if attribute exists within the primitive //
	auto attribute = primitive.attributes.find("POSITION");
	if (attribute == primitive.attributes.end())
	{
		assert(false && "No position attribute found!");
	}

	tinygltf::Accessor& positionAccessor = model.accessors[primitive.attributes.at("POSITION")];
	tinygltf::BufferView& positionView = model.bufferViews[positionAccessor.bufferView];
	tinygltf::Buffer& buffer = model.buffers[positionView.buffer];

	// component = float, int etc. //
	int componentSize = tinygltf::GetComponentSizeInBytes(positionAccessor.componentType);

	// type = vec2, vec3 etc. //
	int typeSize = tinygltf::GetNumComponentsInType(positionAccessor.type);
	int bufferStride = positionAccessor.ByteStride(positionView);

	for (int i = 0; i < positionAccessor.count; i++)
	{
		Vertex vertex;
		size_t bufferLocation = positionView.byteOffset + positionAccessor.byteOffset + (i * bufferStride);

		memcpy(&vertex.Position, &buffer.data[bufferLocation], bufferStride);
		vertices.push_back(vertex);
	}
}

void Mesh::LoadIndices(tinygltf::Model& model, tinygltf::Primitive& primitive)
{
	tinygltf::Accessor& indicesAccessor = model.accessors[primitive.indices];
	tinygltf::BufferView& indicesView = model.bufferViews[indicesAccessor.bufferView];
	tinygltf::Buffer& indicesBuffer = model.buffers[indicesView.buffer];

	int componentSize = tinygltf::GetComponentSizeInBytes(indicesAccessor.componentType);
	int bufferStride = indicesAccessor.ByteStride(indicesView);

	for (int i = 0; i < indicesAccessor.count; i++)
	{
		size_t bufferLocation = indicesView.byteOffset + indicesAccessor.byteOffset + (i * bufferStride);

		if (componentSize == 2)
		{
			short index;
			memcpy(&index, &indicesBuffer.data[bufferLocation], sizeof(short));
			indices.push_back(index);
		}
		
		if (componentSize == 4)
		{
			unsigned int index;
			memcpy(&index, &indicesBuffer.data[bufferLocation], sizeof(unsigned int));
			indices.push_back(index);
		}
	}
}