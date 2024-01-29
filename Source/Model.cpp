#include "Model.h"
#include "Mesh.h"
#include "DXAccess.h"
#include "Logger.h"
#include "Mathematics.h"

Model::Model(const std::string& filePath)
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string error;
	std::string warning;

	Name = filePath.substr(filePath.find_last_of("\\") + 1);

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

void Model::Draw(const glm::mat4& viewProjection)
{
	ComPtr<ID3D12GraphicsCommandList2> commandList =
		DXAccess::GetCommands(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetGraphicsCommandList();

	glm::mat4 MVP = viewProjection * Transform.GetModelMatrix();

	commandList->SetGraphicsRoot32BitConstants(0, 16, &MVP, 0);
	commandList->SetGraphicsRoot32BitConstants(0, 16, &Transform.GetModelMatrix(), 16);

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
	glm::mat4 transform;

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
				matrix.push_back(static_cast<float>(rootNode.matrix[j]));
			}

			transform = glm::make_mat4(matrix.data());
		}
		else
		{
			// Identity Matrix //
			transform = glm::mat4(1.0f);
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

void Model::TraverseChildNodes(tinygltf::Model& model, tinygltf::Node& node, const glm::mat4& parentTransform)
{
	glm::mat4 transform;

	// 1. Load matrix for note //
	if(node.matrix.size() > 0)
	{
		std::vector<float> matrix;
		for(int i = 0; i < 16; i++)
		{
			matrix.push_back(static_cast<float>(node.matrix[i]));
		}

		transform = glm::make_mat4(matrix.data());
	}
	else
	{
		// Identity Matrix //
		transform = glm::mat4(1.0f); 
	}

	glm::mat4 childNodeTransform = parentTransform * transform;

	// 2. Apply to meshes in note //
	if(node.mesh != -1)
	{
		tinygltf::Mesh& mesh = model.meshes[node.mesh];

		for(tinygltf::Primitive& primitive : mesh.primitives)
		{
			meshes.push_back(new Mesh(model, primitive, childNodeTransform));
		}
	}

	// 3. Loop for children // 
	for(int noteID : node.children)
	{
		TraverseChildNodes(model, model.nodes[noteID], childNodeTransform);
	}
}