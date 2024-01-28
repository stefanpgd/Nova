#pragma once

#include <string>
#include <vector>

class Renderer;

// TODO: In the future, if we get many more windows
// make an interface for an editor Window, then loop over
// all windows and render them.
class Editor
{
public:
	Editor(Renderer* renderer);

	void Update(float deltaTime);

private:
	// Editor Windows //
	void ModelSelectionWindow();
	void StatisticsWindow();

	void LoadModelFilePaths(std::string path, std::string originalPath);
	void ImGuiStyleSettings();

private:
	float deltaTime;
	Renderer* renderer;

	// ImGui Editor //
	struct ImFont* baseFont;
	struct ImFont* boldFont;

	// Model Selection //
	std::vector<std::string> modelFilePaths;
	std::vector<std::string> displayNames;
	unsigned int currentModelID = 0;
};