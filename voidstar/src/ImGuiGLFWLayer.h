#pragma once

#include "Window.h"

static void glfw_error_callback(int error, const char* description);
void GLFWSetCallbacks(GLFWwindow* window);

void GLFWSetup();
void ImGuiSetup(GLFWwindow*& window);

void ImGuiStartFrame();
void ImGuiEndFrame();

void ImGuiShutdown();
void GLFWShutdown(GLFWwindow*& window);

void LoadandSetupOpenGL();