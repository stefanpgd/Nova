#include "Engine.h"
#include "resource.h"

#include <cassert>
#include <functional>

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
		switch (message)
		{
		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			switch (wParam)
			{
			case VK_ESCAPE:
				::PostQuitMessage(0);
				break;
			}
		}
		break;
		case WM_SYSCHAR: // To cancel the windows notifaction sound from playing 
			break;

		case WM_SIZE:
		{
			renderer->Resize();
		}
		break;
		case WM_CLOSE:
			::PostQuitMessage(0);
			break;
		default:
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}

	return ::DefWindowProcW(hwnd, message, wParam, lParam);
}