#include "ImGuiGLFWLayer.h"

#include "GLFWCallbacks.h"
#include "Renderer.h"

#include "imgui.h"
#include "GLFW/glfw3.h"

#include <iostream>


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void GLFWSetCallbacks(GLFWwindow* window)
{
    glfwSetWindowCloseCallback(window, WindowCloseCallback);
    glfwSetFramebufferSizeCallback(window, WindowResizeCallback);
    glfwSetKeyCallback(window, KeyPressCallback);
    glfwSetCursorPosCallback(window, MouseMovementCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, MouseScrollCallback);
}


GLFWwindow* GLFWSetup(bool fullscreen)
{
    /* Initialize the library */
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        exit(-1);

    /* GLFW Window Hints */
#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
    // GL 4.3 + GLSL 430
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 16);

    int width;
    int height;
    GLFWmonitor* monitor;

    if (fullscreen)
    {
        // Create a windowed full screen mode window and its OpenGL context
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width = mode->width;
        height = mode->height;
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
    }
    else
    {
        // Normal windowed mode
        width = 1280;
        height = 720;
        monitor = NULL;
    }
    GLFWwindow* window = glfwCreateWindow(width, height, "void*", monitor, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    /* Set callbacks */
    GLFWSetCallbacks(window);

    /* Set mouse to camera controls */
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    return window;
}


void ImGuiSetup(GLFWwindow*& window)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    const char* glsl_version = "#version 430";
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    io.Fonts->AddFontFromFileTTF("res/fonts/Cousine-Regular.ttf", 20.0f);
    
    // Both of these fonts look nice and scale well.
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
}


void ImGuiStartFrame()
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}


void ImGuiEndFrame()
{
    ImGuiIO& io = ImGui::GetIO();

    // Rendering
    ImGui::Render();
    //int display_w, display_h;
    //glfwGetFramebufferSize(window.GetWindow(), &display_w, &display_h);
    //glViewport(0, 0, display_w, display_h);
    //glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    //glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}


void ImGuiShutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


void GLFWShutdown(GLFWwindow*& window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
}


void LoadandSetupOpenGL()
{
    /* Load OpenGL Function Pointers */
    int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    /* Set up blending in OpenGL */
    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    /* Enable MSAA */
    GLCall(glEnable(GL_MULTISAMPLE));

    /* Enable OpenGL Debugging */
#ifdef _DEBUG
    EnableOpenGLDebugging();
#endif
}