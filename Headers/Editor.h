#pragma once

#include <string>
#include <vector>

class Editor
{
public:
	Editor();

private:
	void LoadModelFilePaths(std::string path, std::string originalPath);

	void ImGuiStyleSettings();

private:
	// ImGui Editor //
	struct ImFont* baseFont;
	struct ImFont* boldFont;

	std::vector<std::string> modelFilePaths;
};