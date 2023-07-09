#include "Engine.h"
#include "Renderer.h"
#include "resource.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_win32.h>

#include <cassert>
#include <functional>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Renderer* Engine::renderer;

Engine::Engine()
{
	RegisterWindowClass();
	renderer = new Renderer(applicationName);

	isInitialized = true;
}

Engine::~Engine()
{
	delete renderer;
}

void Engine::Run()
{
	MSG msg = {};

	while (msg.message != WM_QUIT)
	{
		renderer->Render();

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void Engine::RegisterWindowClass()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASSEXW windowClassDescription = {};

	windowClassDescription.cbSize = sizeof(WNDCLASSEX);
	windowClassDescription.style = CS_HREDRAW | CS_VREDRAW;
	windowClassDescription.lpfnWndProc = &WndProc;
	windowClassDescription.cbClsExtra = 0;
	windowClassDescription.cbWndExtra = 0;
	windowClassDescription.hInstance = hInstance;
	windowClassDescription.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	windowClassDescription.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClassDescription.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClassDescription.lpszMenuName = NULL;
	windowClassDescription.lpszClassName = applicationName.c_str();
	windowClassDescription.hIconSm = ::LoadIcon(hInstance, NULL);

	static ATOM atom = ::RegisterClassExW(&windowClassDescription);
	assert(atom > 0);
}

LRESULT Engine::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (renderer)
	{
		ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam);
	}

	return ::DefWindowProcW(hwnd, message, wParam, lParam);
}