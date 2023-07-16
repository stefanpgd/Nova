#pragma once

#include <string>
#include <vector>
#include <tiny_gltf.h>

class Mesh;

class Model
{
public:
	Model(const std::string& fileName);

	void Draw();

	std::vector<Mesh*> meshes;

private:
	void LoadChildren(tinygltf::Model& model, tinygltf::Node& node);
};