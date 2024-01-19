#include "Engine.h"
#include "Renderer.h"

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>
#include <cassert>

#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
	MSG msg = {};
	while(runApplication && msg.message != WM_QUIT)
	{
		if(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}

		renderer->Render();
	}
}

void Engine::RegisterWindowClass()
{
	WNDCLASSEXW windowClassDescription = {};
	HINSTANCE hInstance = GetModuleHandle(NULL);

	windowClassDescription.cbSize = sizeof(WNDCLASSEX);
	windowClassDescription.style = CS_HREDRAW | CS_VREDRAW;
	windowClassDescription.lpfnWndProc = &WindowsCallback;
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

LRESULT Engine::WindowsCallback(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: Add resizing again
	switch(message)
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	}

	// ImGui Windows callback //
	ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam);

	return ::DefWindowProcW(hwnd, message, wParam, lParam);
}