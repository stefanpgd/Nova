#pragma once
#include <string>

class Renderer;

class Engine
{
public:
	Engine(const std::wstring& applicationName);

	void Run();

private:
	void Start();
	void Update();
	void Render();

	void RegisterWindowClass();

private:
	bool runApplication = true;

	Renderer* renderer;

	std::wstring applicationName;
};