#pragma once

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

class Window
{
public:
	Window(bool fullscreen, bool vsync);
	~Window();

	void StartImGuiFrame() const;
	void EndImGuiFrame() const;
	void SwapBuffers() const;
	void PollEvents() const;

	GLFWwindow* CreateWindow(bool fullscreen, bool vsync);
	void SetFullscreen(bool fullscreen);

	void PauseWindow();
	void UnpauseWindow();

	GLFWwindow*& GetWindow() { return m_Window; };

private:

	int m_windowPosx = 100;
	int m_windowPosy = 100;
	int m_windowSizex = 1280;
	int m_windowSizey = 720;

	GLFWmonitor* m_Monitor;
	GLFWwindow* m_Window;
};