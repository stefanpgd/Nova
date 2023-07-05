#pragma once
#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <string>

class Renderer;

class Engine
{
public:
	Engine();
	~Engine();

	void Run();

private:
	void RegisterWindowClass();
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	bool isInitialized = false;

	static Renderer* renderer;
	std::wstring applicationName = L"Nova - A DirectX 12 Renderer";
};
