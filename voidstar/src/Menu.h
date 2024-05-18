#pragma once

void HelpMarker(const char* desc);

class Menu
{
public:
	Menu();
	~Menu();

	void OnUpdate();
	void OnRender();
	void OnImGuiRender();
	void ShowRenderStatistics();
	void PrintRenderStatistics();
	void ShowSceneMenu();
	void ShowSceneDropdown();
	void ShowFileMenu();
	void ShowHelp();
	void ShowAboutModal(bool& showModal);
	void ShowControlsModal(bool& showModal);

private:
	bool m_showFileMenu = true;
	bool m_showRenderStatistics = false;
	bool m_showSceneMenu = false;
	bool m_showCameraControls = false;
	bool m_showFPS = false;
};