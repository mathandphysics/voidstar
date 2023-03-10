#pragma once

class Menu
{
public:
	Menu();
	~Menu();

	void OnUpdate();
	void OnRender();
	void OnImGuiRender();
	void ShowSceneMenu();
	void ShowSceneDropdown();
	void ShowFileMenu();
	void ShowHelp();
	void ShowAboutModal(bool& showModal);
	void ShowControlsModal(bool& showModal);

private:
	bool m_showFileMenu = true;
	bool m_showSceneMenu = false;
	bool m_showCameraControls = false;
	bool m_showFPS = false;
};