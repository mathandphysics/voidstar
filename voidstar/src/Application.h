#pragma once

#include "Window.h"
#include "Renderer.h"
#include "Camera.h"
#include "ScreenshotOverlay.h"
#include "Timer.h"
#include "Menu.h"
#include "scenes/Scene.h"


class Application
{
public:
	Application(const Application&) = delete;
	~Application();

	void Run();
	void OnWindowClose();
	void OnPause();

	static Application& Get()
	{
		static Application s_Instance;
		return s_Instance;
	}
	Window& GetWindow() { return m_Window; }
	Menu& GetMenu() { return m_Menu; }
	scene::SceneManager& GetSceneManager() { return m_SceneManager; }
	Camera& GetCamera() { return m_Camera; }
	Timer& GetTimer() { return m_Timer; }

	void ResetCameraMousePos();
	void SetScreenshotTaken(const std::string& fileName);

	void LoadBlackHole();

private:
	Application();

public:

private:
	bool m_Running = true;
	bool m_Minimized = false;
	bool m_Paused = true;
	Window m_Window;
	Renderer& m_Renderer = Renderer::Get();
	Menu m_Menu;
	Camera m_Camera;
	Timer m_Timer;

	ScreenshotOverlay m_screenshotOverlay;	

	scene::SceneManager m_SceneManager;
};