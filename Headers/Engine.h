#pragma once

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <string>

class Renderer;

class Engine
{
public:
	Engine(const std::wstring& applicationName);

	void Run();

private:
	void RegisterWindowClass();
	static LRESULT CALLBACK WindowsCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
	bool runApplication = true;

	Renderer* renderer;

	std::wstring applicationName;
};