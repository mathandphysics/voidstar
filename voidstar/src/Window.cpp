#include "Window.h"

#include "ImGuiGLFWLayer.h"

Window::Window(bool fullscreen, bool vsync)
    : m_Window(CreateWindow(fullscreen, vsync))
{
    ImGuiSetup(m_Window);

    LoadandSetupOpenGL();

    glfwGetWindowSize(m_Window, &m_width, &m_height);
}


Window::~Window()
{
    ImGuiShutdown();

    GLFWShutdown(m_Window);
}


void Window::StartImGuiFrame() const
{
    ImGuiStartFrame();
}

void Window::EndImGuiFrame() const
{
    ImGuiEndFrame();
}


void Window::SwapBuffers() const
{
    glfwSwapBuffers(m_Window);
}

void Window::PollEvents() const
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
}

void Window::OnImGuiRender() const
{
    ImGui::Text("Resolution: %d x %d", m_width, m_height);
}

GLFWwindow* Window::CreateWindow(bool fullscreen, bool vsync)
{
    GLFWSetup();

    // GLFW Window Hints
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
    // GL 4.6 + GLSL 460
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_SAMPLES, 16);

    m_Monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);
    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    int width, height;
    GLFWmonitor* monitor;
    if (fullscreen)
    {
        // Create a windowed full screen mode window and its OpenGL context
        width = mode->width;
        height = mode->height;
        monitor = m_Monitor;
    }
    else
    {
        // Normal windowed mode
        width = m_restoreSizex;
        height = m_restoreSizey;
        monitor = nullptr;
    }
    GLFWwindow* window = glfwCreateWindow(width, height, "void*", monitor, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    // Set callbacks
    GLFWSetCallbacks(window);

    // Set mouse to camera controls
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Make the window's context current
    glfwMakeContextCurrent(window);
    glfwSwapInterval(vsync); // Enable vsync

    return window;
}

void Window::SetFullscreen(bool fullscreen)
{
    if (fullscreen)
    {
        // Backup window position and window size
        glfwGetWindowPos(m_Window, &m_restorePosx, &m_restorePosy);
        glfwGetWindowSize(m_Window, &m_restoreSizex, &m_restoreSizey);

        // Get resolution of monitor
        const GLFWvidmode* mode = glfwGetVideoMode(m_Monitor);

        // Switch to full screen
        glfwSetWindowMonitor(m_Window, m_Monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
    }
    else
    {
        // Restore last window size and position
        glfwSetWindowMonitor(m_Window, nullptr, m_restorePosx, m_restorePosy, m_restoreSizex, m_restoreSizey, GLFW_DONT_CARE);
    }
}

void Window::SetVSync(bool vsync) const
{
    glfwSwapInterval(vsync);
}

void Window::OnResize()
{
    int vp_width, vp_height;
    glfwGetFramebufferSize(m_Window, &vp_width, &vp_height);
    glViewport(0, 0, vp_width, vp_height);

    glfwGetWindowSize(m_Window, &m_width, &m_height);
}

void Window::SetIcon(const std::string& iconPath)
{
    m_icon.Load(iconPath);
    glfwSetWindowIcon(m_Window, m_icon.GetCount(), m_icon.GetImages().data());
}

std::vector<double> Window::GetCursorPos()
{
    std::vector<double> cursorpos = { 0.0, 0.0 };
    glfwGetCursorPos(m_Window, &cursorpos[0], &cursorpos[1]);
    return cursorpos;
}

void Window::PauseWindow()
{
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
    }
}

void Window::UnpauseWindow()
{
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
}