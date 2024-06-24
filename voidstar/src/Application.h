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
	void ToggleVsync() const;
	void ToggleFullscreen();
	void OnResize(int width, int height);
	void OnClick(int x, int y);

	static Application& Get()
	{
		static Application s_Instance;
		return s_Instance;
	}
	Window& GetWindow() { return m_Window; }
	Menu& GetMenu() { return m_Menu; }
	scene::SceneManager& GetSceneManager() { return m_SceneManager; }
	Camera& GetCamera() { return m_Camera; }
	Timer& GetTimer() { return m_AppTimer; }
	Timer& GetFrameTimer() { return m_FrameTimer; }
	Timer& GetCPUTimer() { return m_CPUTimer; }
	Timer& GetGPUTimer() { return m_GPUTimer; }
	bool GetPaused() const { return m_Paused; }

	void ResetCameraMousePos();
	void ImGuiPrintRenderStats();
	void SetIcon();
	void SetScreenshotTaken(const std::string& fileName);

	void LoadBlackHole();

private:
	Application();

public:

private:
	bool m_fullScreen = false;
	bool m_Vsync = true;
	bool m_Running = true;
	bool m_Minimized = false;
	bool m_Paused = true;
	Window m_Window;
	Renderer& m_Renderer = Renderer::Get();
	Menu m_Menu;
	scene::SceneManager m_SceneManager;
	Camera m_Camera;
	Timer m_AppTimer;
	Timer m_FrameTimer;
	Timer m_CPUTimer;
	Timer m_GPUTimer;

	ScreenshotOverlay m_screenshotOverlay;

	std::string m_iconPath = "res/icon/voidstar.ico";
	ApplicationIcon m_icon;
};