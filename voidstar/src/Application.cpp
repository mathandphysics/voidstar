#include "Application.h"

#include "Camera.h"

Application::Application()
	: m_Window(m_fullScreen, m_Vsync), m_icon(m_iconPath)
{
	ResetCameraMousePos();
	SetIcon();
}

Application::~Application()
{
	m_Renderer.Shutdown();
}

void Application::Run()
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

		if (m_screenshotOverlay.ShouldShow())
		{
			m_screenshotOverlay.OnImGuiRender();
		}

		m_Window.EndImGuiFrame();
		m_Window.SwapBuffers();
		m_GPUTimer.Stop();
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

void Application::ToggleVsync() const
{
	glfwSwapInterval(m_Vsync);
}

void Application::ToggleFullscreen()
{
	m_Window.SetFullscreen(m_fullScreen);
}

void Application::OnResize(int width, int height)
{
#ifndef NDEBUG
	std::cout << "Resizing application." << std::endl;
#endif
	int vp_width, vp_height;
	glfwGetFramebufferSize(m_Window.GetWindow(), &vp_width, &vp_height);
	glViewport(0, 0, vp_width, vp_height);
	if (width * height > 0)
	{
		m_Camera.SetProj();
		m_SceneManager.OnResize();
	}
}

void Application::ResetCameraMousePos()
{
	double currentMouseX, currentMouseY;
	glfwGetCursorPos(m_Window.GetWindow(), &currentMouseX, &currentMouseY);
	m_Camera.GetInputHandler().SetInitialMouseXY(currentMouseX, currentMouseY);
}

void Application::ImGuiPrintRenderStats()
{
	ImGui::Text("Render statistics:");
	float frameTime = Application::Get().GetFrameTimer().GetAverageDeltaTime();
	ImGui::Text("FPS: %.1f", 1 / frameTime);
	ImGui::Text("Frame Time: %.4fs", frameTime);
	float sceneDrawTime = Application::Get().GetCPUTimer().GetAverageDeltaTime();
	ImGui::Text("CPU Time/frame: %.6fs", sceneDrawTime);
	float GUIDrawTime = Application::Get().GetGPUTimer().GetAverageDeltaTime();
	ImGui::Text("GPU Time/frame: %.6fs", GUIDrawTime);
	if (ImGui::Checkbox("Vsync", &m_Vsync))
	{
		ToggleVsync();
	}
	if (ImGui::Checkbox("Full Screen", &m_fullScreen))
	{
		ToggleFullscreen();
	}
	int width, height;
	glfwGetWindowSize(Application::Get().GetWindow().GetWindow(), &width, &height);
	ImGui::Text("Resolution: %d x %d", width, height);
	
}

void Application::SetIcon()
{
	glfwSetWindowIcon(m_Window.GetWindow(), m_icon.GetCount(), m_icon.GetImages().data());
}

void Application::SetScreenshotTaken(const std::string& fileName)
{
	m_screenshotOverlay.NewScreenshotTaken(fileName);
}

void Application::LoadBlackHole()
{
	m_SceneManager.SetCurrentSceneByName("Black Hole");
}
