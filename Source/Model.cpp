#include "Model.h"
#include "Mesh.h"
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

	TraverseRootNodes(model);
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

void Model::TraverseRootNodes(tinygltf::Model& model)
{
	auto scene = model.scenes[model.defaultScene];
	XMMATRIX transform;

	// Traverse the 'root' nodes from the scene
	for (int i = 0; i < scene.nodes.size(); i++)
	{
		tinygltf::Node& rootNode = model.nodes[scene.nodes[i]];
	
		if (rootNode.matrix.size() > 0)
		{
			// Converting from double to float...
			std::vector<float> matrix;
			for (int j = 0; j < 16; j++)
			{
				matrix.push_back(rootNode.matrix[j]);
			}
	
			transform = XMMATRIX(matrix.data());
		}
		else
		{
			transform = XMMatrixIdentity();
		}

		if (rootNode.mesh != -1)
		{
			meshes.push_back(new Mesh(model, model.meshes[rootNode.mesh], transform));
		}

		// Process Child Nodes //
		for (int noteID : rootNode.children)
		{
			TraverseChildNodes(model, model.nodes[noteID], transform);
		}
	}
}

void Model::TraverseChildNodes(tinygltf::Model& model, tinygltf::Node& node, matrix& parentMatrix)
{
	XMMATRIX transform;

	// 1. Load matrix for note //
	if (node.matrix.size() > 0)
	{
		std::vector<float> matrix;
		for (int i = 0; i < 16; i++)
		{
			matrix.push_back(node.matrix[i]);
		}

		transform = XMMATRIX(matrix.data());
	}
	else
	{
		transform = XMMatrixIdentity();
	}

	transform = XMMatrixMultiply(transform, parentMatrix);

	// 2. Apply to meshes in note //
	if (node.mesh != -1)
	{
		meshes.push_back(new Mesh(model, model.meshes[node.mesh], transform));
	}

	// 3. Loop for children // 
	for (int noteID : node.children)
	{
		TraverseChildNodes(model, model.nodes[noteID], transform);
	}
}