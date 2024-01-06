#include "Engine.h"

#include "Renderer.h"

// Engine contains main loop for the "framework"
// Things like the OS loop can also be contained in here

// Things that need to be initialized here?
// - Renderer
// - Input
// - Audio?
// - ImGui?
Engine::Engine(const std::string& applicationName)
{
	renderer = new Renderer(applicationName);
}

void Engine::Run()
{
	while(runApplication)
	{
		Start();
		Update();
		Render();
	}
}

void Engine::Start()
{
}

void Engine::Update()
{
}

void Engine::Render()
{
}