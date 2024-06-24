#include "GLFWCallbacks.h"

#include "Application.h"
#include "Renderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning( push )
#pragma warning(disable:4996)
#include "stb_image_write.h"
#pragma warning( pop )
#include "imgui.h"

#include <iostream>
#include <chrono>
#include <format>
#include <cmath>


void WindowCloseCallback(GLFWwindow* window)
{
	Application::Get().OnWindowClose();
}


void WindowResizeCallback(GLFWwindow* window, int width, int height)
{
	Application::Get().OnResize(width, height);
}


void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	Application::Get().OnResize(width, height);
}


void KeyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		Application::Get().OnPause();
	}
	else
	{
		Application::Get().GetCamera().GetInputHandler().OnKey(key, action);
	}
}


void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
#ifndef NDEBUG
		double xpos;
		double ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		ImGuiIO& io = ImGui::GetIO();
		if (!io.WantCaptureMouse)
		{
			Application::Get().OnClick((int)std::floor(xpos), (int)std::floor(ypos));
		}
#endif
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		Screenshot();
	}
}


void MouseMovementCallback(GLFWwindow* window, double xpos, double ypos)
{
	Application::Get().GetCamera().GetInputHandler().OnMouseMovement(xpos, ypos);
}


void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	Application::Get().GetCamera().GetInputHandler().OnMouseScroll(yoffset);
}


void Screenshot()
{
	auto const timenow = std::chrono::system_clock::now();
	std::string datetimeStr = std::format("{0:%Y%m%d%H%M%S}", timenow);
	// localtime probably has a '.' in it between seconds and milliseconds/microseconds.  If necessary, remove the '.'.
	std::string::size_type n = datetimeStr.find('.');
	if (n != std::string::npos)
	{
		datetimeStr.erase(n, 1);
	}
	std::string outFileName = "Screenshot_" + datetimeStr + ".png";
#ifndef NDEBUG
	std::cout << outFileName << '\n';
#endif

	GLCall(glPixelStorei(GL_PACK_ALIGNMENT, 1));
	int w, h;
	glfwGetWindowSize(Application::Get().GetWindow().GetWindow(), &w, &h);
#ifndef NDEBUG
	std::cout << "File dimensions: " << w << "x" << h << " pixels." << std::endl;
#endif
	int nSize = w * h * 3;
	std::vector<char> dataBuffer(nSize);

	GLCall(glReadPixels((GLint)0, (GLint)0, (GLint)w, (GLint)h, GL_RGB, GL_UNSIGNED_BYTE, dataBuffer.data()));

	stbi_flip_vertically_on_write(1);
	stbi_write_png(outFileName.c_str(), w, h, 3, dataBuffer.data(), w * 3);
#ifndef NDEBUG
	std::cout << "Image written successfully.\n" << std::endl;
#endif

	Application::Get().SetScreenshotTaken(outFileName);
}