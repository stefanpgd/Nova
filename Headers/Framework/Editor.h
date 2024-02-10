#pragma once

#include <string>
#include <vector>

class Scene;

// TODO: In the future, if we get many more windows
// make an interface for an editor Window, then loop over
// all windows and render them.
class Editor
{
public:
	Editor(Scene* scene);

	void Update(float deltaTime);
	void SetScene(Scene* newScene);

private:
	// Editor Windows //
	void ModelSelectionWindow();
	void StatisticsWindow();

	// TODO: Replace with selection window 
	void TransformWindow();

	void LoadModelFilePaths(std::string path, std::string originalPath);
	void ImGuiStyleSettings();

private:
	float deltaTime;
	Scene* scene;

	// ImGui Editor //
	struct ImFont* baseFont;
	struct ImFont* boldFont;

	// Model Selection //
	std::vector<std::string> modelFilePaths;
	std::vector<std::string> displayNames;
	unsigned int currentModelID = 0;
};