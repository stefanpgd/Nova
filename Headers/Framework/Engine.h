#pragma once

#include <chrono>
#include <string>

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

class Scene;
class Editor;
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

	static LRESULT CALLBACK WindowsCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	bool runApplication = true;
	std::wstring applicationName;

	// Framework Components //
	Scene* activeScene;
	Editor* editor;
	Renderer* renderer;

	// Time //
	float deltaTime = 1.0f;
	std::chrono::high_resolution_clock* clock;
	std::chrono::milliseconds t0;

	// TODO: In the future serialize window data
	// Window //
	unsigned int windowWidth = 1080;
	unsigned int windowHeight = 720;
};

