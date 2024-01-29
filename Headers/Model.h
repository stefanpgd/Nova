#pragma once
#include <string>
#include <vector>
#include <tiny_gltf.h>

#include "Transform.h"
#include "DXUtilities.h"

class Mesh;

class Model
{
public:
	Model(const std::string& filePath);

	void Draw(const glm::mat4& viewProjection);

public:
	Transform Transform;
	std::string Name;

private:
	void TraverseRootNodes(tinygltf::Model& model);
	void TraverseChildNodes(tinygltf::Model& model, tinygltf::Node& node, const glm::mat4& parentMatrix);

	std::vector<Mesh*> meshes;
};