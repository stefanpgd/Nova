#include "Model.h"
#include "Mesh.h"
#include <tiny_gltf.h>
#include <cassert>

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

	for (int i = 0; i < model.meshes.size(); i++)
	{
		meshes.push_back(new Mesh(model, model.meshes[i]));
	}
}