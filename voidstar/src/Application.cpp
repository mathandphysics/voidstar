#include "Application.h"

#include "Camera.h"

Application::Application()
	: m_Window(m_fullScreen, m_Vsync)
{
	ResetCameraMousePos();
	SetIcon();
}

Application::~Application()
{
	m_Renderer.Shutdown();
}


void Application::MainLoop()
{
	while (m_Running)
	{
		m_CPUTimer.Start();
		m_FrameTimer.OnUpdate();
		m_AppTimer.OnUpdate();
		m_Window.PollEvents();
		m_Window.StartImGuiFrame();
		m_Renderer.Clear();

		m_Camera.OnUpdate();
		m_SceneManager.OnUpdate();
		m_CPUTimer.Stop();

		m_GPUTimer.Start();
		m_SceneManager.OnRender();

		if (m_Paused)
		{
			m_SceneManager.OnImGuiRender();
			m_Menu.OnImGuiRender();
		}

		m_screenshotOverlay.OnRender();

		m_Window.EndImGuiFrame();
		m_Window.SwapBuffers();
		m_GPUTimer.Stop();
	}
}


void Application::Run()
{
	Load();
	MainLoop();
}

void Application::OnWindowClose()
{
	m_Running = false;
}

void Application::OnPause()
{
	m_Camera.TogglePause();

	if (m_Paused == false)
	{
		m_Window.PauseWindow();
	}
	else
	{
		m_Window.UnpauseWindow();
		ResetCameraMousePos();
	}

	m_Paused = !m_Paused;

}

void Application::ToggleVsync() const
{
	m_Window.SetVSync(m_Vsync);
}

void Application::ToggleFullscreen(bool toggleVar)
{
	if (toggleVar)
	{
		m_fullScreen = !m_fullScreen;
	}
	m_Window.SetFullscreen(m_fullScreen);
}

void Application::OnResize(int width, int height)
{
#ifndef NDEBUG
	std::cout << "Resizing application to size:" << width << "x" << height << "." << std::endl;
#endif
	m_Window.OnResize();
	if (width * height > 0)
	{
		m_Camera.SetProj();
		m_SceneManager.OnResize();
	}
}

void Application::OnClick(int x, int y)
{
	m_SceneManager.OnClick(x, y);
}

void Application::ResetCameraMousePos()
{
	std::vector<double> currentMousePos = m_Window.GetCursorPos();
	m_Camera.GetInputHandler().SetInitialMouseXY(currentMousePos[0], currentMousePos[1]);
}

void Application::ImGuiPrintRenderStats()
{
	ImGui::Text("Render statistics:");
	float frameTime = m_FrameTimer.GetAverageDeltaTime();
	ImGui::Text("FPS: %.1f", 1 / frameTime);
	ImGui::Text("Frame Time: %.4fs", frameTime);
	float sceneDrawTime = m_CPUTimer.GetAverageDeltaTime();
	ImGui::Text("CPU Time/frame: %.6fs", sceneDrawTime);
	float GUIDrawTime = m_GPUTimer.GetAverageDeltaTime();
	ImGui::Text("GPU Time/frame: %.6fs", GUIDrawTime);
	if (ImGui::Checkbox("Vsync", &m_Vsync))
	{
		ToggleVsync();
	}
	if (ImGui::Checkbox("Full Screen", &m_fullScreen))
	{
		ToggleFullscreen();
	}
	m_Window.OnImGuiRender();
}

void Application::SetIcon()
{
	m_Window.SetIcon(m_iconPath);
}

void Application::SetScreenshotTaken(const std::string& fileName)
{
	m_screenshotOverlay.NewScreenshotTaken(fileName);
}

void Application::Load()
{
	m_SceneManager.SetCurrentSceneByName("Black Hole");
}