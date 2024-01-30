#pragma once
#include <imgui.h>

enum class KeyCode
{
	A = ImGuiKey::ImGuiKey_A,
	B = ImGuiKey::ImGuiKey_B,
	C = ImGuiKey::ImGuiKey_C,
	D = ImGuiKey::ImGuiKey_D,
	E = ImGuiKey::ImGuiKey_E,
	F = ImGuiKey::ImGuiKey_F,
	G = ImGuiKey::ImGuiKey_G,
	H = ImGuiKey::ImGuiKey_H,
	I = ImGuiKey::ImGuiKey_I,
	J = ImGuiKey::ImGuiKey_J,
	K = ImGuiKey::ImGuiKey_K,
	L = ImGuiKey::ImGuiKey_L,
	M = ImGuiKey::ImGuiKey_M,
	N = ImGuiKey::ImGuiKey_N,
	O = ImGuiKey::ImGuiKey_O,
	P = ImGuiKey::ImGuiKey_P,
	Q = ImGuiKey::ImGuiKey_Q,
	R = ImGuiKey::ImGuiKey_R,
	S = ImGuiKey::ImGuiKey_S,
	T = ImGuiKey::ImGuiKey_T,
	U = ImGuiKey::ImGuiKey_U,
	V = ImGuiKey::ImGuiKey_V,
	W = ImGuiKey::ImGuiKey_W,
	X = ImGuiKey::ImGuiKey_X,
	Y = ImGuiKey::ImGuiKey_Y,
	Z = ImGuiKey::ImGuiKey_Z,

	Shift = ImGuiKey::ImGuiKey_ModShift,
	Ctrl = ImGuiKey::ImGuiKey_ModCtrl,
	Escape = ImGuiKey::ImGuiKey_Escape
};

enum class MouseCode
{
	Left = 0,
	Right,
	Middle
};

class Input
{
public:
	static bool GetKey(KeyCode key);
	static bool GetMouseButton(MouseCode button);

	static int GetMouseX();
	static int GetMouseY();
	static int GetMouseVelocityX();
	static int GetMouseVelocityY();
};