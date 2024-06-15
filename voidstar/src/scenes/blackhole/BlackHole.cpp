#include "BlackHole.h"

#include "Application.h"
#include <cmath>



graphicsPreset defaultPreset = { 0.95f, 0.8f, 2.5f, 1.0f, 0.7f, 1.0f, 3.0f, 2.0f, 4400.0f, 0.0f };
graphicsPreset interstellarPreset = { 1.6f, 0.1f, 10.0f, 0.49f, 0.56f, 1.0f, 3.0f, 1.9f, 2800.0f, 385.0f };
graphicsPreset shadowPreset = { 0.7f, 0.1f, 4.8f, 0.43f, 0.19f, 0.0f, 5.2f, 0.0f, 40000.0f, 5000.0f };
graphicsPreset celshadePreset = { 2.0f, 0.1f, 10.0f, 1.40f, 0.10f, 0.0f, 3.0f, 3.0f, 2100.0f, 385.0f };


BlackHole::BlackHole()
{
    CalculateISCO();
    CreateScreenQuad();

    LoadTextures();
    CompileBHShaders();
    CreateFBOs();
    CompilePostShaders();

    // y=1 puts the camera slightly above the xz-plane of the accretion disk.
    Application::Get().GetCamera().SetCameraPos(glm::vec3(0.0f, 1.0f, -26.0f));
    //Application::Get().GetCamera().SetCameraPos(glm::vec3(0.0f, 0.0f, 0.4f));
    SetProjectionMatrix();

    SetGraphicsPreset(interstellarPreset);
}

BlackHole::~BlackHole()
{
}

void BlackHole::OnUpdate()
{
    float deltaTime = Application::Get().GetTimer().GetDeltaTime();
    m_diskRotationAngle -= deltaTime * m_diskRotationSpeed;

    // Set the new draw distance based on camera position.  Also determines whether the camera is inside the event horizon.
    m_drawDistance = CalculateDrawDistance();
}

void BlackHole::Draw()
{
    // Draw to initial off-screen FBO
    m_fbo->Bind();
    m_quad.SetShader(m_selectedShaderString, m_vertexDefines, m_fragmentDefines);
    SetShaderUniforms();
    m_quad.Draw();
    m_fbo->Unbind();

    // Post-processing off-screen
    PostProcess();

    // Final draw to screen from FBO
    m_quad.SetShader(m_BloomShaderPath);
    SetScreenShaderUniforms();
    m_quad.Draw();
}

void BlackHole::CreateScreenQuad()
{
    // Make a rectangle that exactly fills the screen.  Then just use the fragment shader to draw on it.

    struct mesh_with_tc {
        glm::vec3 pos;
        glm::vec2 tc;
    };
    glm::vec3 v1 = glm::vec3(-1.0f, -1.0f, 0.0f);
    glm::vec2 tc1 = glm::vec2(0.0f, 0.0f);
    glm::vec3 v2 = glm::vec3(1.0f, -1.0f, 0.0f);
    glm::vec2 tc2 = glm::vec2(1.0f, 0.0f);
    glm::vec3 v3 = glm::vec3(1.0f, 1.0f, 0.0f);
    glm::vec2 tc3 = glm::vec2(1.0f, 1.0f);
    glm::vec3 v4 = glm::vec3(-1.0f, 1.0f, 0.0f);
    glm::vec2 tc4 = glm::vec2(0.0f, 1.0f);
    std::vector<mesh_with_tc> positions({ mesh_with_tc{v1, tc1},
                                          mesh_with_tc{v2, tc2},
                                          mesh_with_tc{v3, tc3},
                                          mesh_with_tc{v4, tc4} });

    unsigned int triangles[] = {
        0, 1, 2,
        2, 3, 0
    };

    VertexBufferLayout layout;
    layout.Push<float>(3);
    layout.Push<float>(2);
    SetShaderDefines();
    m_quad.Initialize((const float*)&positions[0], 4, layout, triangles, 6, m_selectedShaderString, 
        "", true, m_vertexDefines, m_fragmentDefines);
    //m_quad.SetPosition(glm::vec3(0.0f, 0.0f, 1.0f / glm::tan(glm::radians(60.0f) / 2.0f)));
    m_quad.SetPosition(glm::vec3(0.0f, 0.0f, 1.0f / glm::tan(glm::radians(m_FOVy) / 2.0f)));
    m_quad.SetProjection(m_proj, false);
}

void BlackHole::LoadTextures()
{
    // Cube map for skybox
    // Ordering of faces must be: xpos, xneg, ypos, yneg, zpos, zneg.
    m_cubeTexturePaths = {
        "res/textures/px.png",
        "res/textures/nx.png",
        "res/textures/py.png",
        "res/textures/ny.png",
        "res/textures/pz.png",
        "res/textures/nz.png"
    };
    m_cubemap.SetCubeMap(m_cubeTexturePaths);

    m_diskTexturePath = "res/textures/accretion_disk.jpg";
    m_diskTexture = Renderer::Get().GetTexture(m_diskTexturePath, true);

    if (m_useSphereTexture)
    {
        m_sphereTexturePath = "res/textures/Blue_Marble_2002.png";
        m_sphereTexture = Renderer::Get().GetTexture(m_sphereTexturePath, true);
    }
}

void BlackHole::CompileBHShaders()
{
    // Load and compile the black hole shaders and set uniforms.
    SetShaderDefines();
    SetShader(m_kerrBlackHoleShaderPath);
}

void BlackHole::CompilePostShaders()
{
    // Load and compile the post-processing shaders and set uniforms.
    std::shared_ptr<Shader> texture_to_screen_shader = Renderer::Get().GetShader(m_textureToScreenShaderPath);
    texture_to_screen_shader->Bind();
    texture_to_screen_shader->SetUniform1i("screenTexture", m_screenTextureSlot);
    GLint vp[4];
    GLCall(glGetIntegerv(GL_VIEWPORT, vp));
    texture_to_screen_shader->SetUniform4f("u_ScreenSize", (float)vp[0], (float)vp[1], (float)vp[2], (float)vp[3]);

    std::shared_ptr<Shader> blur_shader = Renderer::Get().GetShader(m_gaussianBlurShaderPath);
    blur_shader->Bind();
    blur_shader->SetUniform1i("screenTexture", m_screenTextureSlot);
    blur_shader->SetUniform4f("u_ScreenSize", (float)vp[0], (float)vp[1], (float)vp[2], (float)vp[3]);

    std::shared_ptr<Shader> bloom_shader = Renderer::Get().GetShader(m_BloomShaderPath);
    bloom_shader->Bind();
    bloom_shader->SetUniform1i("screenTexture", m_screenTextureSlot);
    bloom_shader->SetUniform1i("blurTexture", m_screenTextureSlot + 1);
    bloom_shader->SetUniform4f("u_ScreenSize", (float)vp[0], (float)vp[1], (float)vp[2], (float)vp[3]);
}

void BlackHole::CreateFBOs()
{
    // Make the FBOs.  These are used for post-processing and bloom in particular.
    FramebufferSpecification fbospec;
    int width, height;
    glfwGetWindowSize(Application::Get().GetWindow().GetWindow(), &width, &height);
    fbospec.height = height;
    fbospec.width = width;
    fbospec.numColouredAttachments = 1;
    m_pingFBO = std::make_shared<Framebuffer>(fbospec);
    m_pingFBO->Unbind();
    m_pongFBO = std::make_shared<Framebuffer>(fbospec);
    m_pongFBO->Unbind();
    fbospec.numColouredAttachments = 2;
    m_fbo = std::make_shared<Framebuffer>(fbospec);
    m_fbo->Unbind();
}

void BlackHole::SetShaderUniforms()
{
    GLint vp[4];
    GLCall(glGetIntegerv(GL_VIEWPORT, vp));

    std::shared_ptr<Shader> shader = m_quad.GetShader();
    shader->Bind();

    float elapsedTime = Application::Get().GetTimer().GetElapsedTime();
    shader->SetUniform1f("u_Time", elapsedTime);
    shader->SetUniform4f("u_ScreenSize", (float)vp[0], (float)vp[1], (float)vp[2], (float)vp[3]);
    shader->SetUniform1f("u_InnerRadius", m_diskInnerRadius);
    shader->SetUniform1f("u_OuterRadius", m_diskOuterRadius);
    shader->SetUniform1f("u_BHRadius", m_radius);
    shader->SetUniform1f("u_risco", m_risco);
    shader->SetUniform1f("u_fOfRISCO", m_fOfRISCO);
    shader->SetUniform1f("u_BHMass", m_mass);
    shader->SetUniform1f("u_a", m_a);
    shader->SetUniform1f("u_Tmax", m_Tmax);
    shader->SetUniform1f("u_diskRotationAngle", m_diskRotationAngle);

    shader->SetUniform1i("u_msaa", m_msaa);

    shader->SetUniform1i("u_maxSteps", m_maxSteps);
    shader->SetUniform1f("u_stepSize", m_stepSize);
    shader->SetUniform1f("u_drawDistance", m_drawDistance);
    shader->SetUniform1f("u_epsilon", m_epsilon);
    shader->SetUniform1i("u_ODESolver", m_ODESolverSelector);
    shader->SetUniform1f("u_tolerance", m_tolerance);
    shader->SetUniform1f("u_diskIntersectionThreshold", m_diskIntersectionThreshold);
    shader->SetUniform1f("u_sphereIntersectionThreshold", m_sphereIntersectionThreshold);

    shader->SetUniform1i("diskTexture", m_diskTextureSlot);
    shader->SetUniform1i("skybox", m_skyboxTextureSlot);
    shader->SetUniform1i("sphereTexture", m_sphereTextureSlot);
    shader->SetUniform1i("u_useSphereTexture", m_useSphereTexture);
    shader->SetUniform1i("u_useDebugSphereTexture", (int)m_useDebugSphereTexture);
    shader->SetUniform1i("u_drawBasicDisk", m_drawBasicDisk);
    shader->SetUniform1i("u_transparentDisk", m_transparentDisk);
    shader->SetUniform1i("u_useDebugDiskTexture", (int)m_useDebugDiskTexture);
    shader->SetUniform3f("u_sphereDebugColour1", m_sphereDebugColour1[0], m_sphereDebugColour1[1], m_sphereDebugColour1[2]);
    shader->SetUniform3f("u_sphereDebugColour2", m_sphereDebugColour2[0], m_sphereDebugColour2[1], m_sphereDebugColour2[2]);
    shader->SetUniform3f("u_diskDebugColourTop1", m_diskDebugColourTop1[0], m_diskDebugColourTop1[1], m_diskDebugColourTop1[2]);
    shader->SetUniform3f("u_diskDebugColourTop2", m_diskDebugColourTop2[0], m_diskDebugColourTop2[1], m_diskDebugColourTop2[2]);
    shader->SetUniform3f("u_diskDebugColourBottom1", m_diskDebugColourBottom1[0], m_diskDebugColourBottom1[1], m_diskDebugColourBottom1[2]);
    shader->SetUniform3f("u_diskDebugColourBottom2", m_diskDebugColourBottom2[0], m_diskDebugColourBottom2[1], m_diskDebugColourBottom2[2]);

    shader->SetUniform1i("u_bloom", m_useBloom);
    shader->SetUniform1f("u_bloomThreshold", m_bloomThreshold);
    shader->SetUniform1f("u_diskAbsorption", m_diskAbsorption);
    shader->SetUniform1f("u_bloomBackgroundMultiplier", m_bloomBackgroundMultiplier);
    shader->SetUniform1f("u_bloomDiskMultiplier", m_bloomDiskMultiplier);
    shader->SetUniform1f("u_brightnessFromRadius", m_brightnessFromRadius);
    shader->SetUniform1f("u_brightnessFromDiskVel", m_brightnessFromDiskVel);
    shader->SetUniform1f("u_colourshiftPower", m_colourshiftPower);
    shader->SetUniform1f("u_colourshiftMultiplier", m_colourshiftMultiplier);
    shader->SetUniform1f("u_colourshiftOffset", m_colourshiftOffset);
    shader->SetUniform1f("u_exposure", m_exposure);
    shader->SetUniform1f("u_gamma", m_gamma);

    glm::vec3 cameraPos = Application::Get().GetCamera().GetPosition();
    shader->SetUniform3f("u_cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);

    m_diskTexture->Bind(m_diskTextureSlot);
    if (m_useSphereTexture)
    {
        m_sphereTexture->Bind(m_sphereTextureSlot);
    }
    m_cubemap.Bind(m_skyboxTextureSlot);
}

void BlackHole::SetScreenShaderUniforms()
{
    std::shared_ptr<Shader> shader = m_quad.GetShader();
    shader->Bind();
    GLint vp[4];
    GLCall(glGetIntegerv(GL_VIEWPORT, vp));
    shader->SetUniform4f("u_ScreenSize", (float)vp[0], (float)vp[1], (float)vp[2], (float)vp[3]);
    shader->SetUniform1i("u_bloom", m_useBloom);
    shader->SetUniform1f("u_exposure", m_exposure);
    shader->SetUniform1f("u_gamma", m_gamma);
    GLCall(glActiveTexture(GL_TEXTURE0 + m_screenTextureSlot));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_fbo->GetColourAttachments()[0]));
    GLCall(glActiveTexture(GL_TEXTURE0 + m_screenTextureSlot + 1));
    if (m_horizontalPass)
    {
        GLCall(glBindTexture(GL_TEXTURE_2D, m_pongFBO->GetColourAttachments()[0]));
    }
    else
    {
        GLCall(glBindTexture(GL_TEXTURE_2D, m_pingFBO->GetColourAttachments()[0]));
    }
}

void BlackHole::PostProcess()
{
    if (m_useBloom)
    {
        // Do Guassian blur
        m_horizontalPass = true;
        m_firstIteration = true;
        unsigned int amount = 10;
        m_quad.SetShader(m_gaussianBlurShaderPath);
        std::shared_ptr<Shader> shader = m_quad.GetShader();
        shader->Bind();
        GLint vp[4];
        GLCall(glGetIntegerv(GL_VIEWPORT, vp));
        shader->SetUniform4f("u_ScreenSize", (float)vp[0], (float)vp[1], (float)vp[2], (float)vp[3]);
        shader->SetUniform1i("u_horizontal", m_horizontalPass);
        for (unsigned int i = 0; i < amount; i++)
        {
            if (m_horizontalPass)
            {
                m_pingFBO->Bind();
                GLCall(GLCall(glActiveTexture(GL_TEXTURE0 + m_screenTextureSlot)));
                if (m_firstIteration)
                {
                    GLCall(glBindTexture(GL_TEXTURE_2D, m_fbo->GetColourAttachments()[1]));
                }
                else
                {
                    GLCall(glBindTexture(GL_TEXTURE_2D, m_pongFBO->GetColourAttachments()[0]));
                }
            }
            else
            {
                m_pongFBO->Bind();
                GLCall(GLCall(glActiveTexture(GL_TEXTURE0 + m_screenTextureSlot)));
                if (m_firstIteration)
                {
                    GLCall(glBindTexture(GL_TEXTURE_2D, m_fbo->GetColourAttachments()[1]));
                }
                else
                {
                    GLCall(glBindTexture(GL_TEXTURE_2D, m_pingFBO->GetColourAttachments()[0]));
                }
            }

            m_quad.Draw();

            m_horizontalPass = !m_horizontalPass;
            if (m_firstIteration)
            {
                m_firstIteration = false;
            }
            
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

float BlackHole::CalculateKerrDistance(glm::vec3 p)
{
    float b = m_a * m_a - glm::dot(p, p);
    float c = -m_a * m_a * p.y * p.y;
    float r2 = 0.5f * (-b + sqrt(b * b - 4.0f * c));
    return sqrt(r2);
}

float BlackHole::CalculateDrawDistance()
{
    // Update the max distance to draw from the origin based on the camera's current coordinates
    // depending on the current metric.

    glm::vec3 p = Application::Get().GetCamera().GetPosition();
    switch (m_shaderSelector)
    {
    case 0:
    {
        // Kerr black hole.
        // r is the "distance" in the Kerr-Schild coordinates.
        float r = CalculateKerrDistance(p);
        bool horizonTest = (r < m_mass + sqrt(m_mass * m_mass - m_a * m_a));
        if (horizonTest != m_insideHorizon)
        {
            m_insideHorizon = horizonTest;
            if (m_insideHorizon == true)
            {
                m_tolerance = 0.00005f;
                m_maxSteps = 40;
            }
            else
            {
                m_tolerance = 0.01f;
                m_maxSteps = 200;
            }
            SetShaderDefines();
        }
        return fmax(70.0f, r + 10.0f);
        break;
    }
    default:
        // Flat space and classical BH.
        float r = sqrt(glm::dot(p, p));
        m_insideHorizon = (r < 2 * m_mass);
        return fmax(70.0f, r + 10.0f);
        break;
    }
}

void BlackHole::CalculateISCO()
{
    // Calculate in the innermost stable circular orbit for a black hole with parameters mass m and rotation a.
    // Result is the theoretically smallest inner radius for a stable accretion disk.
    // Assumes G = c = 1
    if (m_mass == 0)
    {
        // In this case, there is no stable orbit because there is no mass.  So the result is actually infinity.
        m_risco = 0.0f;
    }
    float rs = 2.0f * m_mass;
    float chi = 2.0f * m_a / rs;
    float z1 = 1.0f + std::powf(1.0f - chi*chi, 1.0f/3.0f) * (std::powf(1.0f + chi, 1.0f/3.0f) + std::powf(1.0f - chi, 1.0f/3.0f));
    float z2 = std::powf(3.0f*chi*chi + z1*z1, 1.0f / 2.0f);
    m_risco = m_mass * (3.0f + z2 - std::powf((3.0f-z1)*(3.0f+z1+2.0f*z2), 1.0f / 2.0f));

    // Now calculate f(risco)
    // Uses the black hole's mass and spin, along with a point on the equatorial accretion disk that's r away from the
    // center of the black hole, to calculate f, a function used in the time-averaged flux of radiant energy
    // from "Disk-accretion onto a black hole" by Page and Thorne (1973)
    float astar = m_a / m_mass;
    float x = sqrt((49.0f / 36.0f) * m_risco / m_mass);
    float x0 = sqrt(m_risco / m_mass);
    float pi = 3.141592653589793f;
    // x1, x2, x3 are the roots of x^3 - 3*x + 2*astar = 0.
    float x1 = 2.0f * cosf((1.0f / 3.0f) * acosf(astar) - pi / 3.0f);
    float x2 = 2.0f * cosf((1.0f / 3.0f) * acosf(astar) + pi / 3.0f);
    float x3 = -2.0f * cosf((1.0f / 3.0f) * acosf(astar));
    float x1numer = 3.0f * (x1 - astar) * (x1 - astar);
    float x1denom = x1 * (x1 - x2) * (x1 - x3);
    float x2numer = 3.0f * (x2 - astar) * (x2 - astar);
    float x2denom = x2 * (x2 - x1) * (x2 - x3);
    float x3numer = 3.0f * (x3 - astar) * (x3 - astar);
    float x3denom = x3 * (x3 - x1) * (x3 - x2);
    float xdenom = x * x * (x * x * x - 3.0f * x + 2.0f * astar);
    float logx0 = logf(x / x0);
    float logx1 = logf((x - x1) / (x0 - x1));
    float logx2 = logf((x - x2) / (x0 - x2));
    float logx3 = logf((x - x3) / (x0 - x3));
    m_fOfRISCO = (3.0f / (2.0f * m_mass)) * (1.0f / xdenom) * (x - x0 - (3.0f * astar / 2.0f) * logx0
        - (x1numer / x1denom) * logx1 - (x2numer / x2denom) * logx2
        - (x3numer / x3denom) * logx3);

}

void BlackHole::OnImGuiRender()
{
    if (m_setImGuiPosition)
    {
        // Fix the controls window to the right side of the screen.
        ImVec2 work_size = ImGui::GetWindowViewport()->WorkSize;
        ImVec2 window_size = ImVec2(340, work_size.y);
        ImVec2 window_pos = ImGui::GetWindowViewport()->WorkPos;
        window_pos[0] += work_size.x - window_size[0];
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
        m_setImGuiPosition = !m_setImGuiPosition;
    }

    ImGuiWindowFlags imgui_window_flags = ImGuiWindowFlags_NoCollapse;
    imgui_window_flags |= ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;
    imgui_window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
    imgui_window_flags |= ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("Black Hole Controls", nullptr, imgui_window_flags);
    ImGui::PushItemWidth(-FLT_MIN);
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("##BHTabBar", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Simulation"))
        {
            ImGuiRenderStats();
            ImGui::Separator();
            ImGui::Separator();

            ImGuiChooseBH();
            ImGui::Separator();
            ImGui::Separator();

            ImGuiBHProperties();
            ImGui::Separator();
            ImGui::Separator();

            ImGuiChooseODESolver();
            ImGui::Separator();
            ImGui::Separator();

            ImGuiSimQuality();
            ImGui::Separator();
            ImGui::Separator();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lighting/Colour"))
        {
            ImGuiDebug();
            ImGui::Separator();
            ImGui::Separator();

            ImGuiCinematic();
            ImGui::Separator();
            ImGui::Separator();

            ImGui::EndTabItem();
        }

#ifndef NDEBUG
        if (ImGui::BeginTabItem("Camera Debug"))
        {
            Application::Get().GetCamera().DebugGUI();

            ImGui::Separator();
            ImGui::Separator();

            ImGui::EndTabItem();
        }
#endif

        ImGui::EndTabBar();
        ImGui::PopItemWidth();
    }
    ImGui::End();
}

void BlackHole::ImGuiRenderStats()
{
    Application::Get().ImGuiPrintRenderStats();
    ImGui::Text("Anti-Aliasing: WARNING!");
    ImGui::SameLine();
    HelpMarker("MSAA is a very expensive operation for a ray marcher.  As a rule of thumb, if your uncapped framerate at "
                "MSAA=1 is F, then your framerate at MSAA=x will be roughly F/(x*x).  So, for example, going from MSAA=1 "
                "to MSAA=3 will reduce framerate by a factor of 9.");
    ImGui::SliderInt("##MSAA", &m_msaa, 1, 4, "MSAA = %d");
}

void BlackHole::ImGuiChooseBH()
{
    ImGui::Text("Black Hole Selector:");
    if (ImGui::RadioButton("Kerr BH", &m_shaderSelector, 0))
    {
        m_useBloom = true;
        m_selectedShaderString = m_kerrBlackHoleShaderPath;
        SetShader(m_selectedShaderString);
        m_maxSteps = 200;
        m_a = 0.6f;
        CalculateISCO();
    }
    ImGui::SameLine();
    HelpMarker("Accurate rotating black hole using the Kerr metric of General Relativity.  "
        "The parameter \"a\" above is the spin parameter.  a=0 is a non-rotating black hole.");
    if (ImGui::RadioButton("Classical BH (Fast)", &m_shaderSelector, 1))
    {
        m_useBloom = false;
        m_selectedShaderString = m_kerrBlackHoleShaderPath;
        SetShader(m_selectedShaderString);
        m_presetSelector = -1;
        SetGraphicsPreset(defaultPreset);
        m_maxSteps = 2000;
        m_a = 0.0f;
        CalculateISCO();
    }
    ImGui::SameLine();
    HelpMarker("Inaccurate, non-rotating black hole based on Newtonian Gravity.");
    if (ImGui::RadioButton("Minkowski BH", &m_shaderSelector, 2))
    {
        m_useBloom = false;
        m_selectedShaderString = m_kerrBlackHoleShaderPath;
        SetShader(m_selectedShaderString);
        m_maxSteps = 2000;
        m_a = 0.0;
        CalculateISCO();
    }
    ImGui::SameLine();
    HelpMarker("Minkowski metric of General Relativity.  This is standard Euclidean 3-space.");
    if (ImGui::RadioButton("Flat BH (Fast)", &m_shaderSelector, 3))
    {
        m_useBloom = false;
        m_selectedShaderString = m_flatShaderPath;
        SetShader(m_selectedShaderString);
    }
    ImGui::SameLine();
    HelpMarker("Ray tracer using straight line rays/intersections.");
}

void BlackHole::ImGuiBHProperties()
{
    ImGui::Text("Black Hole Properties:");
    ImGui::SameLine();
    HelpMarker("Increasing Black Hole Mass increases the curving of space.  From the observer's perspective,"
        " it's almost like zooming in.  The \"a\" parameter is a measure of the black hole's spin and only "
        "works on the Kerr black hole.  Larger a results in a more squished black hole.  It might also "
        "cause graphical artifacts to appear in the black hole's center.  If this happens, increase the "
        "simulation quality below.");
    if (ImGui::SliderFloat("##BlackHoleMass", &m_mass, 1.0, 3.0, "Black Hole Mass = %.1f"))
    {
        m_radius = 2.0f * m_mass;
        m_diskInnerRadius = 3.0f * m_radius;
        if (m_mass <= m_a)
        {
            m_a = m_mass - 0.05f, 0.0f;
            if (m_a < 0.0f)
            {
                m_a = 0.0f;
            }
        }
        CalculateISCO();
    }
    ImGui::SliderFloat("##InnerDiskRadius", &m_diskInnerRadius, 1.5f * m_radius, 5.0f * m_radius, "Inner Disk Radius = %.1f");
    if (ImGui::Button("Set Inner Radius to ISCO"))
    {
        m_diskInnerRadius = m_risco;
    }
    ImGui::SliderFloat("##OuterDiskRadius", &m_diskOuterRadius, m_diskInnerRadius, 10.0f * m_radius,
        "Outer Disk Radius = %.1f");
    if (ImGui::SliderFloat("##a", &m_a, 0.0f, fmin(m_mass - 0.01f, 0.99f), "a = %.2f"))
    {
        CalculateISCO();
    }
    ImGui::SliderFloat("##Max Temperature", &m_Tmax, 1000.0f, 20000.0f, "Max Disk Temperature = %.0f");
    ImGui::SliderFloat("##DiskAbsorption", &m_diskAbsorption, 0.0f, 7.0f, "Disk Absorption = %.1f");
    ImGui::SliderFloat("##DiskRotationSpeed", &m_diskRotationSpeed, 0.0f, 0.3f, "Disk Rotation Speed = %.3f");
}

void BlackHole::ImGuiChooseODESolver()
{
    ImGui::Text("ODE Solver:");
    if (ImGui::RadioButton("Forward-Euler-Cromer", &m_ODESolverSelector, 0))
    {
        SetShader(m_selectedShaderString);
    }
    ImGui::SameLine();
    HelpMarker("Least accurate, but fast.");
    if (ImGui::RadioButton("Classic RK4", &m_ODESolverSelector, 1))
    {
        SetShader(m_selectedShaderString);
    }
    ImGui::SameLine();
    HelpMarker("More accurate than Forward-Euler-Cromer, but it doesn't choose stepsize intelligently.");
    if (ImGui::RadioButton("Adaptive RK2/3", &m_ODESolverSelector, 2))
    {
        SetShader(m_selectedShaderString);
    }
    ImGui::SameLine();
    HelpMarker("Bogacki-Shampine Method.  Use the tolerance slider to choose a trade-off between speed and accuracy.");
    if (ImGui::RadioButton("Adaptive RK4/5", &m_ODESolverSelector, 3))
    {
        SetShader(m_selectedShaderString);
    }
    ImGui::SameLine();
    HelpMarker("Dormand-Prince Method.  Use the tolerance slider to choose a trade-off between speed and accuracy."
                "  Note that because of its high accuracy, this solver takes very large steps.  This can cause issues with "
                "detecting that the rays hit the accretion disk.  As a result, the inner edge of the disk may look "
                "jagged.");
}

void BlackHole::ImGuiSimQuality()
{
    ImGui::Text("Simulation Quality:");
    ImGui::SameLine();
    HelpMarker("Making Step Size smaller will increase the accuracy of the simulation.  However, it "
        "can also make the ring or black hole disappear.  If this happens, increase Max Steps to "
        "make everything reappear with greater accuracy.  WARNING: this will significantly increase "
        "computational cost and slow down the simulation.");
    ImGui::SliderInt("##MaxSteps", &m_maxSteps, 1, 1000, "Max Steps = %d");
    ImGui::SliderFloat("##Disk Intersection Threshold", &m_diskIntersectionThreshold, 0.0001f, 0.1f, "Disk Intersection Error = %.4f");
    ImGui::SliderFloat("##Sphere Intersection Threshold", &m_sphereIntersectionThreshold, 0.0001f, 0.1f, "Sphere Intersection Error = %.4f");
    //ImGui::SliderFloat("##StepSize", &m_stepSize, 0.01f, 0.5f, "Step Size = %.2f");
    //ImGui::SliderFloat("##drawDistance", &m_drawDistance, 1.0f, 1000.0, "Max Distance = %.0f");
    //ImGui::SliderFloat("##Epsilon", &m_epsilon, 0.00001f, 0.001f, "Epsilon = %.5f");
    if (m_ODESolverSelector == 2 || m_ODESolverSelector == 3)
    {
        ImGui::Text("Tolerance:");
        ImGui::SameLine();
        HelpMarker("Tolerance is the maximum allowed local error in the integration step.  A smaller tolerance leads to"
        " a more accurate simulation.");
        ImGui::SliderFloat("##Tolerance", &m_tolerance, 0.00005f, 0.01f, "Tolerance = %.5f");
    }
}

void BlackHole::ImGuiDebug()
{
    ImGui::Text("Debug Properties:");
    ImGui::Checkbox("Don't Light/Colour Disk", &m_drawBasicDisk);
    ImGui::Checkbox("Disk Debug Texture", &m_useDebugDiskTexture);
    ImGui::Checkbox("Sphere Debug Texture", &m_useDebugSphereTexture);

    if (m_useDebugDiskTexture)
    {
        ImGui::Text("Disk Top Debug Colour 1");
        ImGui::ColorEdit3("##ColourTop1", &m_diskDebugColourTop1[0]);
        ImGui::Text("Disk Top Debug Colour 2");
        ImGui::ColorEdit3("##ColourTop2", &m_diskDebugColourTop2[0]);
        ImGui::Text("Disk Bottom Debug Colour 1");
        ImGui::ColorEdit3("##ColourBottom1", &m_diskDebugColourBottom1[0]);
        ImGui::Text("Disk Bottom Debug Colour 2");
        ImGui::ColorEdit3("##ColourBottom2", &m_diskDebugColourBottom2[0]);
    }
    if (m_useDebugSphereTexture)
    {
        ImGui::Text("Sphere Debug Colour 1");
        ImGui::ColorEdit3("##SphereColour1", &m_sphereDebugColour1[0]);
        ImGui::Text("Sphere Debug Colour 2");
        ImGui::ColorEdit3("##SphereColour2", &m_sphereDebugColour2[0]);
    }
}

void BlackHole::ImGuiCinematic()
{
    if (ImGui::Checkbox("Cinematic Mode", &m_useBloom))
    {
        if (m_useBloom)
        {
            m_presetSelector = 0;
            SetGraphicsPreset(interstellarPreset);
        }
        else
        {
            m_presetSelector = -1;
            SetGraphicsPreset(defaultPreset);
        }
    }
    if (m_useBloom)
    {
        ImGui::Separator();
        ImGui::Text("Graphics Presets:");
        if (ImGui::RadioButton("Interstellar-esque", &m_presetSelector, 0))
        {
            SetGraphicsPreset(interstellarPreset);
        }
        if (ImGui::RadioButton("Pale Shadow", &m_presetSelector, 1))
        {
            SetGraphicsPreset(shadowPreset);
        }
        if (ImGui::RadioButton("Quasi Cel-Shaded", &m_presetSelector, 2))
        {
            SetGraphicsPreset(celshadePreset);
        }
        if (ImGui::RadioButton("Simple", &m_presetSelector, 3))
        {
            SetGraphicsPreset(defaultPreset);
        }
        ImGui::Separator();
        ImGui::SliderFloat("##BloomThreshold", &m_bloomThreshold, 0.1f, 10.0f, "Bloom Threshold = %.1f");
        ImGui::SliderFloat("##BackgroundMultiplier", &m_bloomBackgroundMultiplier, 0.0f, 10.0f, "Background = %.1f");
        ImGui::SliderFloat("##DiskMultiplier", &m_bloomDiskMultiplier, 0.0f, 10.0f, "Disk = %.1f");
        ImGui::SliderFloat("##Exposure", &m_exposure, 0.1f, 4.0f, "Exposure = %.2f");
        ImGui::SliderFloat("##Gamma", &m_gamma, 0.1f, 3.0f, "Gamma = %.2f");
        ImGui::SliderFloat("##brightnessFromRadius", &m_brightnessFromRadius, 0.0f, 10.0f, "Brightness from Radius = %.2f");
        ImGui::SliderFloat("##BrightnessFromDiskVel", &m_brightnessFromDiskVel, 0.0f, 10.0f, "Disk Velocity Brightness = %.1f");
        ImGui::SliderFloat("##ColourshiftPower", &m_colourshiftPower, 0.0f, 10.0f, "Disk Colour Power = %.1f");
        ImGui::SliderFloat("##ColourshiftMultiplier", &m_colourshiftMultiplier, 100.0f, 40000.0f, "Disk Colour Multiplier = %.0f");
        ImGui::SliderFloat("##ColourshiftOffset", &m_colourshiftOffset, 0.0f, 5000.0f, "Disk Colour Offset = %.0f");

    }
}

void BlackHole::OnResize()
{
    CreateFBOs();
    SetProjectionMatrix();
    m_setImGuiPosition = true;
}

void BlackHole::SetProjectionMatrix()
{
    int width, height;
    glfwGetWindowSize(Application::Get().GetWindow().GetWindow(), &width, &height);
    m_proj = glm::perspective(glm::radians(m_FOVy), (float)width / (float)height, 0.1f, 1.0f);
    m_quad.SetProjection(m_proj, false);
}

void BlackHole::SetGraphicsPreset(const graphicsPreset &preset)
{
    m_bloomThreshold = preset.bloomThreshold;
    m_bloomBackgroundMultiplier = preset.bloomBackgroundMultiplier;
    m_bloomDiskMultiplier = preset.bloomDiskMultiplier;
    m_exposure = preset.exposure;
    m_gamma = preset.gamma;
    m_brightnessFromRadius = preset.brightnessFromRadius;
    m_brightnessFromDiskVel = preset.brightnessFromDiskVel;
    m_colourshiftPower = preset.colourshiftPower;
    m_colourshiftMultiplier = preset.colourshiftMultiplier;
    m_colourshiftOffset = preset.colourshiftOffset;
}

void BlackHole::SetShader(const std::string& filePath)
{
    // Load and compile, if necessary, the black hole shader and set uniforms.
    //std::shared_ptr<Shader> shader = Renderer::Get().GetShader(filePath);
    SetShaderDefines();
    std::shared_ptr<Shader> shader = Renderer::Get().GetShader(filePath, m_vertexDefines, m_fragmentDefines);
    shader->Bind();
    SetShaderUniforms();
}

void BlackHole::SetShaderDefines()
{
    m_vertexDefines.clear();
    m_fragmentDefines.clear();

    switch (m_shaderSelector)
    {
    case 0:
        // Kerr black hole
        m_fragmentDefines.push_back("KERR");
        m_fragmentDefines.push_back("ODE_SOLVER " + std::to_string(m_ODESolverSelector));
        if (m_insideHorizon)
        {
            m_fragmentDefines.push_back("INSIDE_HORIZON");
        }
        break;
    case 1:
        // Classical black hole
        m_fragmentDefines.push_back("CLASSICAL");
        m_fragmentDefines.push_back("ODE_SOLVER " + std::to_string(m_ODESolverSelector));
        break;
    case 2:
        // Minkowski black hole
        m_fragmentDefines.push_back("MINKOWSKI");
        m_fragmentDefines.push_back("ODE_SOLVER " + std::to_string(m_ODESolverSelector));
        break;
    case 3:
        // Classical ray-traced flatspace.
        break;
    }
}
