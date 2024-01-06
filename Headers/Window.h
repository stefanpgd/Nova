#pragma once
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class Window
{
public:
	Window(const std::wstring& applicationName, unsigned int windowWidth, unsigned int windowHeight);

	unsigned int GetCurrentBackBufferIndex();

public:
	static const unsigned int BackBufferCount = 3;

private:
	void CheckTearingSupported();
	void SetupWindowRect(const std::wstring& applicationName);

	HWND windowHandle;
	RECT windowRect;

	unsigned int windowWidth;
	unsigned int windowHeight;
};