#pragma once
#include <string>
#include <vector>
#include <tiny_gltf.h>

#include "Graphics/Transform.h"
#include "Graphics/DXUtilities.h"

class Mesh;

class Model
{
public:
	Model(const std::string& filePath);

	void Draw(const glm::mat4& viewProjection, const glm::mat4& lightMatrix);

	Mesh* GetMesh(int index);
	const std::vector<Mesh*>& GetMeshes();

public:
	Transform Transform;
	std::string Name;

private:
	void TraverseRootNodes(tinygltf::Model& model);
	void TraverseChildNodes(tinygltf::Model& model, tinygltf::Node& node, const glm::mat4& parentMatrix);

	std::vector<Mesh*> meshes;
};