#include "Mesh.h"
#include "DXAccess.h"
#include <cassert>

Mesh::Mesh(tinygltf::Model& model, tinygltf::Mesh& mesh)
{
	// A 'Mesh' exists out of multiple primitives, usually this is one
	// but it can be more. Each primitive contains the geometry data (triangles, lines etc. )
	// to render the model
	for (int i = 0; i < mesh.primitives.size(); i++)
	{
		LoadAttribute(model, mesh.primitives[i], "POSITION");
		LoadAttribute(model, mesh.primitives[i], "NORMAL");
		LoadIndices(model, mesh.primitives[i]);
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

void Mesh::LoadAttribute(tinygltf::Model& model, tinygltf::Primitive& primitive, const std::string& attributeType)
{
	auto attribute = primitive.attributes.find(attributeType);

	// Check if within the primitives's attributes the type is present. For example 'Normals'
	// If not, stop here, the model isn't valid
	if (attribute == primitive.attributes.end())
	{
		assert(false && "Attribute not available");
	}

	// Accessor: Tells use which view we need, what type of data is in it, and the amount/count of data.
	// BufferView: Tells which buffer we need, and where we need to be in the buffer
	// Buffer: Binary data of our mesh
	tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at(attributeType)];
	tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
	tinygltf::Buffer& buffer = model.buffers[view.buffer];

	// Object: data structure that got created using components, such as a vec3 ( uses 3 floats )
	unsigned int objectSize = accessor.ByteStride(view);

	// In case it hasn't happened, resize the vertex buffer since we're 
	// going to directly memcpy the data into an already existing buffer
	if (vertices.size() < accessor.count)
	{
		vertices.resize(accessor.count);
	}

	for (int i = 0; i < accessor.count; i++)
	{
		Vertex& vertex = vertices[i];
		size_t bufferLocation = view.byteOffset + (i * objectSize);

		// TODO: Add 'offsetto' to this and use pointers to vertices instead of a reference
		if (attributeType == "POSITION")
		{
			memcpy(&vertex.Position, &buffer.data[bufferLocation], objectSize);
		}
		else if (attributeType == "NORMAL")
		{
			memcpy(&vertex.Normal, &buffer.data[bufferLocation], objectSize);
		}
	}
}

void Mesh::LoadIndices(tinygltf::Model& model, tinygltf::Primitive& primitive)
{
	tinygltf::Accessor& accessor = model.accessors[primitive.indices];
	tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
	tinygltf::Buffer& buffer = model.buffers[view.buffer];

	unsigned int componentSize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
	unsigned int objectSize = accessor.ByteStride(view);

	for (int i = 0; i < accessor.count; i++)
	{
		unsigned int bufferLocation = view.byteOffset + (i * objectSize);

		// TODO: Test if casting directly might work?
		if (componentSize == 2)
		{
			short index;
			memcpy(&index, &buffer.data[bufferLocation], sizeof(short));
			indices.push_back(index);
		}

		if (componentSize == 4)
		{
			unsigned int index;
			memcpy(&index, &buffer.data[bufferLocation], sizeof(unsigned int));
			indices.push_back(index);
		}
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
