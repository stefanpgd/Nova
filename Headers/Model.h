#pragma once

#include <string>
#include <vector>

class Mesh;

class Model
{
public:
	Model(const std::string& fileName);
	std::vector<Mesh*> meshes;

private:
};