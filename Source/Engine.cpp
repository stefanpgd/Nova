#include "Engine.h"
#include "Renderer.h"

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

#include <cassert>

// Engine contains main loop for the "framework"
// Things like the OS loop can also be contained in here

// Things that need to be initialized here?
// - Renderer
// - Input
// - Audio?
// - ImGui?
Engine::Engine(const std::wstring& applicationName) : applicationName(applicationName)
{
	RegisterWindowClass();

	renderer = new Renderer(this->applicationName);
}

void Engine::Run()
{
	while(runApplication)
	{

	}
}

void Engine::RegisterWindowClass()
{
	WNDCLASSEXW windowClassDescription = {};
	HINSTANCE hInstance = GetModuleHandle(NULL);

	windowClassDescription.cbSize = sizeof(WNDCLASSEX);
	windowClassDescription.style = CS_HREDRAW | CS_VREDRAW;
	//windowClassDescription.lpfnWndProc = &WndProc;
	windowClassDescription.cbClsExtra = 0;
	windowClassDescription.cbWndExtra = 0;
	windowClassDescription.hInstance = hInstance;
	windowClassDescription.hIcon = ::LoadIcon(hInstance, NULL);
	windowClassDescription.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClassDescription.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClassDescription.lpszMenuName = NULL;
	windowClassDescription.lpszClassName = applicationName.c_str();
	windowClassDescription.hIconSm = ::LoadIcon(hInstance, NULL);

	static ATOM atom = ::RegisterClassExW(&windowClassDescription);
	assert(atom > 0);
}