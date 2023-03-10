#pragma once

#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window
{
public:
	Window();
	~Window();

	void StartImGuiFrame() const;
	void EndImGuiFrame() const;
	void SwapBuffers() const;
	void PollEvents() const;

	void PauseWindow();
	void UnpauseWindow();

	GLFWwindow*& GetWindow() { return m_Window; };

private:
	GLFWwindow* m_Window;
};