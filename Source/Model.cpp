#include "Model.h"
#include "Mesh.h"
#include <cassert>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/matrix_decompose.hpp>

Model::Model(const std::string& fileName)
{
	tinygltf::TinyGLTF loader;
	tinygltf::Model model;
	std::string warning;
	std::string error;

	if (!loader.LoadASCIIFromFile(&model, &error, &warning, fileName))
	{
		assert(false && "Failed to load glTF file");
	}

	if (!warning.empty())
	{
		printf("Model Loading Warning: %s", warning.c_str());
	}

	if (!error.empty())
	{
		printf("Model Loading ERROR: %s", error.c_str());
	}

	// 1. Grab default scene, maybe in the future loop over all scenes if models
	// happen to be incomplete.
	auto& scene = model.scenes[model.defaultScene];
	
	// 2. Loop over every node in the scene
	for (int i = 0; i < scene.nodes.size(); i++)
	{
		tinygltf::Node& node = model.nodes[i];

		// 3. If the node contains a mesh, load it in together with the
		// passed along hierachy matrix. 
		if (node.mesh != -1)
		{
			tinygltf::Mesh& mesh = model.meshes[node.mesh];

			// get some custom node matrix...

			for (int i = 0; i < mesh.primitives.size(); i++)
			{
				meshes.push_back(new Mesh(model, mesh.primitives[i]));
			}
		}

		// 4. If there are any childeren attached to the node, load those in
		// using the parents matrix to properly offset everything
		for (int i = 0; i < node.children.size(); i++)
		{
			tinygltf::Node& childNode = model.nodes[node.children[i]];
			LoadChildren(model, childNode);
		}
	}
}

void Model::Draw()
{
	for (int i = 0; i < meshes.size(); i++)
	{
		meshes[i]->SetAndDraw();
	}
}

void Model::LoadChildren(tinygltf::Model& model, tinygltf::Node& node)
{
	if (node.mesh != -1)
	{
		tinygltf::Mesh& mesh = model.meshes[node.mesh];

		// get some custom node matrix...

		for (int i = 0; i < mesh.primitives.size(); i++)
		{
			meshes.push_back(new Mesh(model, mesh.primitives[i]));
		}
	}

	for (int i = 0; i < node.children.size(); i++)
	{
		tinygltf::Node& childNode = model.nodes[node.children[i]];
		LoadChildren(model, childNode);
	}
}