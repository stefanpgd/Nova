#include "Mesh.h"
#include "DXUtilities.h"
#include "DXAccess.h"
#include "DXCommands.h"

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