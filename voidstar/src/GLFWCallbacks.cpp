#include "GLFWCallbacks.h"

#include "Application.h"
#include "Renderer.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning( push )
#pragma warning(disable:4996)
#include "stb_image_write.h"
#pragma warning( pop )

#include <iostream>
#include <chrono>


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
		;
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
	std::time_t time = std::time({});
	std::tm calendar;
	localtime_s(&calendar, &time); // This is Microsoft only -_-.  Needed for thread safety.
	char timeString[std::size("yyyymmddhhmmss")];  // date string will be yyyymmddhhmmss, so 14 long +1 terminating char.
	std::strftime(std::data(timeString), std::size(timeString), "%Y%m%d%H%M%S", &calendar);
	timeString[14] = '\0';  // Guarantee timeString is null terminated for the string constructor.
	std::string outFileName = timeString;
	outFileName = "Screenshot_" + outFileName + ".png";
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

	GLCall(glReadPixels((GLint)0, (GLint)0, (GLint)w, (GLint)h, GL_RGB, GL_UNSIGNED_BYTE, &dataBuffer[0]));

	stbi_flip_vertically_on_write(1);
	stbi_write_png(outFileName.c_str(), w, h, 3, dataBuffer.data(), w * 3);
#ifndef NDEBUG
	std::cout << "Image written successfully.\n" << std::endl;
#endif

	Application::Get().SetScreenshotTaken(outFileName);
}