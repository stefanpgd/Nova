#include "Model.h"
#include "Mesh.h"
#include "DXAccess.h"
#include "Logger.h"

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
		LOG(Log::MessageType::Debug, warning);
	}

	if(!error.empty())
	{
		LOG(Log::MessageType::Error, error);
	}

	if(!result)
	{
		assert(false && "Failed to parse model.");
	}

	TraverseRootNodes(model);
}

void Model::Draw(matrix& viewProjection)
{
	ComPtr<ID3D12GraphicsCommandList2> commandList =
		DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetGraphicsCommandList();

	matrix MVP = XMMatrixMultiply(Transform.GetModelMatrix(), viewProjection);
	commandList->SetGraphicsRoot32BitConstants(0, 16, &MVP, 0);

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
	for(int i = 0; i < scene.nodes.size(); i++)
	{
		tinygltf::Node& rootNode = model.nodes[scene.nodes[i]];

		if(rootNode.matrix.size() > 0)
		{
			// Converting from double to float...
			std::vector<float> matrix;
			for(int j = 0; j < 16; j++)
			{
				matrix.push_back(rootNode.matrix[j]);
			}

			transform = XMMATRIX(matrix.data());
		}
		else
		{
			transform = XMMatrixIdentity();
		}

		if(rootNode.mesh != -1)
		{
			tinygltf::Mesh& mesh = model.meshes[rootNode.mesh];

			for(tinygltf::Primitive& primitive : mesh.primitives)
			{
				meshes.push_back(new Mesh(model, primitive, transform));
			}
		}

		// Process Child Nodes //
		for(int noteID : rootNode.children)
		{
			TraverseChildNodes(model, model.nodes[noteID], transform);
		}
	}
}

void Model::TraverseChildNodes(tinygltf::Model& model, tinygltf::Node& node, matrix& parentMatrix)
{
	XMMATRIX transform;

	// 1. Load matrix for note //
	if(node.matrix.size() > 0)
	{
		std::vector<float> matrix;
		for(int i = 0; i < 16; i++)
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
	if(node.mesh != -1)
	{
		tinygltf::Mesh& mesh = model.meshes[node.mesh];

		for(tinygltf::Primitive& primitive : mesh.primitives)
		{
			meshes.push_back(new Mesh(model, primitive, transform));
		}
	}

	// 3. Loop for children // 
	for(int noteID : node.children)
	{
		TraverseChildNodes(model, model.nodes[noteID], transform);
	}
}