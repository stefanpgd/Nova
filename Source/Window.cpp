#include "Window.h"

#include <cassert>

Window::Window(const std::wstring& applicationName, unsigned int windowWidth, unsigned int windowHeight) :
	windowWidth(windowWidth), windowHeight(windowHeight)
{
	SetupWindowRect(applicationName);
}

unsigned int Window::GetCurrentBackBufferIndex()
{
	// swapChain->GetCurrentBackBufferIndex();
	return 0;
}

void Window::CheckTearingSupported()
{
	// Move to swap chain...

	// Variable refresh rate display (GSync, FreeSync) need to have vsync off
	// to be able to run, so we need to check if tearing (vsync off) is supported
	//ComPtr<IDXGIFactory5> factory5;
	//ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory5)));

	//if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
	//{
	//	allowTearing = FALSE;
	//}
}

void Window::SetupWindowRect(const std::wstring& applicationName)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(windowWidth), static_cast<LONG>(windowHeight) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	int windowX = max(0, (screenWidth - windowWidth) / 2);
	int windowY = max(0, (screenHeight - windowHeight) / 2);

	windowHandle = ::CreateWindowExW(NULL, applicationName.c_str(), applicationName.c_str(), WS_OVERLAPPEDWINDOW,
		windowX, windowY, windowWidth, windowHeight, NULL, NULL, hInstance, nullptr);

	assert(windowHandle && "Failed to create window handle");

	GetWindowRect(windowHandle, &windowRect);
}