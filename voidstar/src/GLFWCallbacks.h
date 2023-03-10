#pragma once

#include "Window.h"

void WindowCloseCallback(GLFWwindow* window);
void WindowResizeCallback(GLFWwindow* window, int width, int height);
void KeyPressCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void MouseMovementCallback(GLFWwindow* window, double xpos, double ypos);
void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

void Screenshot();