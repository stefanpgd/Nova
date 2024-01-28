#include "Input.h"

bool Input::GetKey(KeyCode key)
{
	return ImGui::IsKeyDown(ImGuiKey(key));
}

bool Input::GetMouseButton(MouseCode button)
{
	return ImGui::GetIO().MouseDown[unsigned int(button)];
}

// TODO: Maybe add checks to stay within the window
int Input::GetMouseX()
{
	return ImGui::GetIO().MousePos.x;
}

int Input::GetMouseY()
{
	return ImGui::GetIO().MousePos.y;
}

int Input::GetMouseVelocityX()
{
	return ImGui::GetIO().MouseDelta.x;
}

int Input::GetMouseVelocityY()
{
	return ImGui::GetIO().MouseDelta.y;
}