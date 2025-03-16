#pragma once

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "Texture.h"

class Window
{
public:
	Window(bool fullscreen, bool vsync);
	~Window();

	void StartImGuiFrame() const;
	void EndImGuiFrame() const;
	void SwapBuffers() const;
	void PollEvents() const;
	void OnImGuiRender() const;

	GLFWwindow* CreateWindow(bool fullscreen, bool vsync);
	void SetFullscreen(bool fullscreen);
	void SetVSync(bool vsync) const;
	void OnResize();
	void SetIcon(const std::string& iconPath);

	std::vector<double> GetCursorPos();

	void PauseWindow();
	void UnpauseWindow();

	GLFWwindow*& GetGLFWWindow() { return m_Window; };

private:

	int m_restorePosx = 100;
	int m_restorePosy = 100;
	int m_restoreSizex = 1280;
	int m_restoreSizey = 720;
	int m_width = 0;
	int m_height = 0;

	GLFWmonitor* m_Monitor;
	GLFWwindow* m_Window;
	ApplicationIcon m_icon;
};