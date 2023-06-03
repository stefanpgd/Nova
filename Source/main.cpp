#pragma once
#define WIN32_LEAN_AND_MEAN // reduces the amount of headers included from Windows.h
#include <Windows.h>

#include "Renderer.h"

// STL Headers
#include <iostream>
#include <cassert>
#include <chrono>

// Window message callback
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void RegisterWindowClass(HINSTANCE hInst, std::wstring windowName)
{
	WNDCLASSEXW windowClassDescription = {};

	windowClassDescription.cbSize = sizeof(WNDCLASSEX);
	windowClassDescription.style = CS_HREDRAW | CS_VREDRAW;
	windowClassDescription.lpfnWndProc = &WndProc;
	windowClassDescription.cbClsExtra = 0;
	windowClassDescription.cbWndExtra = 0;
	windowClassDescription.hInstance = hInst;
	windowClassDescription.hIcon = ::LoadIcon(hInst, NULL);
	windowClassDescription.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClassDescription.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClassDescription.lpszMenuName = NULL;
	windowClassDescription.lpszClassName = windowName.c_str();
	windowClassDescription.hIconSm = ::LoadIcon(hInst, NULL);

	static ATOM atom = ::RegisterClassExW(&windowClassDescription);
	assert(atom > 0);
}

void Update()
{
	static uint64_t frameCounter = 0;
	static double elapsedSeconds = 0.0;
	static std::chrono::high_resolution_clock clock;
	static auto t0 = clock.now();

	frameCounter++;
	auto t1 = clock.now();
	auto deltaTime = t1 - t0;
	t0 = t1;

	elapsedSeconds += deltaTime.count() * 1e-9;
	if (elapsedSeconds > 1.0)
	{
		auto fps = frameCounter / elapsedSeconds;
		printf("FPS: %f\n", fps);

		frameCounter = 0;
		elapsedSeconds = 0.0;
	}
}

Renderer* renderer;

int main()
{
	std::wstring applicationName = L"Nova";

	HINSTANCE hInstance = GetModuleHandle(NULL);
	RegisterWindowClass(hInstance, applicationName);

	renderer = new Renderer(applicationName);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		Update();
		renderer->Render();

		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return 0;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
		// The default window procedure will play a system notification sound 
		// when pressing the Alt+Enter keyboard combination if this message is 
		// not handled.
		case WM_SYSCHAR:
			break;

		case WM_SIZE:
		{
			renderer->Resize();
		}
		break;
		case WM_DESTROY:
			::PostQuitMessage(0);
			break;
		default:
			return ::DefWindowProcW(hwnd, message, wParam, lParam);
		}
	}

	return ::DefWindowProcW(hwnd, message, wParam, lParam);
}