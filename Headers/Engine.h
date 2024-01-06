#pragma once
#include <string>

class Renderer;

class Engine
{
public:
	Engine(const std::string& applicationName);

	void Run();

private:
	void Start();
	void Update();
	void Render();

private:
	bool runApplication = true;

	Renderer* renderer;

	std::string applicationName;
};