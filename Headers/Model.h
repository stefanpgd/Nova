#pragma once

#include <string>
#include <vector>

class Mesh;

class Model
{
public:
	Model(const std::string& filePath);

	void Draw();

private:

	std::vector<Mesh*> meshes;
};