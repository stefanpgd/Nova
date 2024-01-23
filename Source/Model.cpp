#include "Model.h"
#include "Mesh.h"
#include "tiny_gltf.h"

#include "DXAccess.h"

Model::Model(const std::string& filePath)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string error;
	std::string warning;

	// Tiny glTF provides us with a model
	// the model structure contains EVERYTHING already neatly prepared in vectors.
	bool result = loader.LoadASCIIFromFile(&model, &error, &warning, filePath);

	if(!warning.empty())
	{
		printf("Warning: %s\n", error.c_str());
	}

	if(!error.empty())
	{
		printf("Error: %s\n", error.c_str());
	}

	if(!result)
	{
		assert(false && "Failed to parse model.");
	}

	for(int i = 0; i < model.meshes.size(); i++)
	{
		meshes.push_back(new Mesh(model, model.meshes[i]));
	}
}

void Model::Draw()
{
	ComPtr<ID3D12GraphicsCommandList2> commandList = 
		DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetGraphicsCommandList();

	for(Mesh* mesh : meshes)
	{
		commandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
		commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());

		commandList->DrawIndexedInstanced(mesh->GetIndicesCount(), 1, 0, 0, 0);
	}
}