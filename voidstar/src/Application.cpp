#include "Application.h"

#include "Camera.h"

Application::Application()
{
	ResetCameraMousePos();
}

Application::~Application()
{
	m_Renderer.Shutdown();
}

void Application::Run()
{
	while (m_Running)
	{
		m_FrameTimer.OnUpdate();
		m_Timer.OnUpdate();
		m_Window.PollEvents();
		m_Window.StartImGuiFrame();
		m_Renderer.Clear();

		m_Camera.OnUpdate();
		m_SceneManager.OnUpdate();

		m_SceneDrawTimer.Start();
		m_SceneManager.OnRender();
		m_SceneDrawTimer.Stop();

		m_GUIDrawTimer.Start();
		if (m_Paused)
		{
			m_SceneManager.OnImGuiRender();
			m_Menu.OnImGuiRender();
		}

		if (m_screenshotOverlay.ShouldShow())
		{
			m_screenshotOverlay.OnImGuiRender();
		}

		m_Window.EndImGuiFrame();
		m_Window.SwapBuffers();
		m_GUIDrawTimer.Stop();
	}
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

void Application::ResetCameraMousePos()
{
	double currentMouseX, currentMouseY;
	glfwGetCursorPos(m_Window.GetWindow(), &currentMouseX, &currentMouseY);
	m_Camera.GetInputHandler().SetInitialMouseXY(currentMouseX, currentMouseY);
}

void Application::SetScreenshotTaken(const std::string& fileName)
{
	m_screenshotOverlay.NewScreenshotTaken(fileName);
}

void Application::LoadBlackHole()
{
	m_SceneManager.SetCurrentSceneByName("Black Hole");
}
