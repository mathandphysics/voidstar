#include "BlackHole.h"

#include "Application.h"
#include <cmath>
#include "imgui_internal.h"

BlackHole::BlackHole()
{
    CalculateISCO();
    CreateScreenQuad();

    LoadTextures();
    CompileBHShaders();
    CreateFBOs();
    CompilePostShaders();

    // y=1 puts the camera slightly above the xz-plane of the accretion disk.
    // Ideally we'd be able to set the z-distance from the black hole programmatically based on the camera's FOV,
    // but because of the bending of light, the calculation too complicated, so we set it manually.
    // FOV = 60  =>  z-distance = -25.0.
    // FOV = 40  =>  z-distance = -35.5.
    Application::Get().GetCamera().SetCameraPos(glm::vec3(0.0f, 1.0f, -35.5f));

    SetGraphicsPreset(m_presets[m_presetSelector]);
}

BlackHole::~BlackHole()
{
}

void BlackHole::OnUpdate()
{
    SetProjectionMatrix();
    float deltaTime = Application::Get().GetTimer().GetDeltaTime();
    m_diskRotationAngle -= deltaTime * m_diskRotationSpeed;

    // Set the new draw distance based on camera position.  Also determine whether the camera is inside the event horizon.
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
    m_quad.Initialize((const float*)positions.data(), 4, layout, triangles, 6, m_selectedShaderString,
        "", true, m_vertexDefines, m_fragmentDefines);
    //m_quad.SetPosition(glm::vec3(0.0f, 0.0f, 1.0f / glm::tan(glm::radians(60.0f) / 2.0f)));
    float fov = Application::Get().GetCamera().GetFOV();
    m_quad.SetPosition(glm::vec3(0.0f, 0.0f, 1.0f / glm::tan(glm::radians(fov) / 2.0f)));
    SetProjectionMatrix();
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

    // Set disk texture path here, if desired.
    m_diskTexturePath = "";
    if (!m_diskTexturePath.empty())
    {
        m_diskTexture = Renderer::Get().GetTexture(m_diskTexturePath, true);
    }

    if (m_useSphereTexture)
    {
        // Set sphere texture path here, if desired.
        m_sphereTexturePath = "";
        if (!m_sphereTexturePath.empty())
        {
            m_sphereTexture = Renderer::Get().GetTexture(m_sphereTexturePath, true);
        }
    }
}

void BlackHole::CompileBHShaders()
{
    // Load and compile the black hole shaders and set uniforms.
    SetShaderDefines();
    SetShader(m_kerrBlackHoleShaderPath);
}

void BlackHole::CompilePostShaders() const
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
    shader->SetUniform1f("u_BHMass", m_mass);
    shader->SetUniform1f("u_dMdt", m_dMdt);
    shader->SetUniform1f("u_a", m_a);
    shader->SetUniform1f("u_Tmax", m_Tmax);
    shader->SetUniform1f("u_diskRotationAngle", m_diskRotationAngle);

    shader->SetUniform1i("u_msaa", m_msaa);

    shader->SetUniform1i("u_maxSteps", m_maxSteps);
    shader->SetUniform1f("u_drawDistance", m_drawDistance);
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
    shader->SetUniform1f("u_brightnessFromDiskVel", m_brightnessFromDiskVel);
    shader->SetUniform1f("u_blueshiftPower", m_blueshiftPower);
    shader->SetUniform1f("u_exposure", m_exposure);
    shader->SetUniform1f("u_gamma", m_gamma);

    glm::vec3 cameraPos = Application::Get().GetCamera().GetPosition();
    shader->SetUniform3f("u_cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);

    if (m_diskTexture)
    {
        m_diskTexture->Bind(m_diskTextureSlot);
    }
    if (m_useSphereTexture && m_sphereTexture)
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
        // Guassian blur
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

float BlackHole::CalculateKerrDistance(const glm::vec3 p) const
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
            if (m_shaderSelector == 0)
            {
                if (m_insideHorizon == true)
                {
                    m_tolerance = 0.01f;
                    m_maxSteps = 70;
                }
                else
                {
                    m_tolerance = 0.01f;
                    m_maxSteps = 200;
                }
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
}

void BlackHole::OnImGuiRender()
{
    ImGuiSetUpDocking();

    ImGuiWindowFlags imgui_window_flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoCollapse;
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

void BlackHole::ImGuiSetUpDocking()
{
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_HiddenTabBar;
    if (!m_ImGuiAllowMoveableDock)
    {
        dockspace_flags |= ImGuiDockNodeFlags_NoUndocking | ImGuiDockNodeFlags_NoTabBar;
    }
    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), dockspace_flags);

    if (m_ImGuiFirstTime)
    {
        m_ImGuiFirstTime = false;
        ImGui::DockBuilderRemoveNode(dockspace_id); // clear any previous layout
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);
        auto dock_id_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.25f, nullptr, &dockspace_id);
        ImGui::DockBuilderDockWindow("Black Hole Controls", dock_id_right);
    }

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
        m_selectedShaderString = m_kerrBlackHoleShaderPath;
        SetShader(m_selectedShaderString);
        m_maxSteps = 200;
        m_a = 0.6f;
        CalculateISCO();
    }
    ImGui::SameLine();
    HelpMarker("Accurate rotating black hole using the Kerr metric of General Relativity.  "
        "The parameter \"a\" above is the spin parameter.  a=0 is a non-rotating black hole.");

    if (ImGui::RadioButton("Minkowski BH", &m_shaderSelector, 2))
    {
        m_selectedShaderString = m_kerrBlackHoleShaderPath;
        SetShader(m_selectedShaderString);
        m_maxSteps = 2000;
        m_a = 0.0;
        CalculateISCO();
    }
    ImGui::SameLine();
    HelpMarker("Minkowski metric of General Relativity.  This is standard Euclidean 3-space.");
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
    ImGui::Text("Graphics Presets:");
    for (int i = 0; i < m_presets.size(); i++)
    {
        if (ImGui::RadioButton(m_presets[i].name.c_str(), &m_presetSelector, i))
        {
            SetGraphicsPreset(m_presets[i]);
        }
    }
    ImGui::Separator();
    ImGui::Separator();
    ImGui::SliderFloat("##BackgroundMultiplier", &m_bloomBackgroundMultiplier, 0.0f, 10.0f, "Background = %.1f");
    ImGui::SliderFloat("##DiskMultiplier", &m_bloomDiskMultiplier, 0.0f, 10.0f, "Disk = %.1f");
    ImGui::SliderFloat("##dMdt", &m_dMdt, 0.0f, 10000.0f, "dM/dt = %.0f");
    ImGui::SliderFloat("##blueshiftPower", &m_blueshiftPower, 0.0f, 10.0f, "Blueshift Power = %.2f");
    ImGui::SliderFloat("##BrightnessFromDiskVel", &m_brightnessFromDiskVel, 0.0f, 10.0f, "Disk Velocity Brightness = %.1f");
    ImGui::Separator();
    ImGui::Separator();
    if (ImGui::Checkbox("Cinematic Mode", &m_useBloom))
    {
        if (m_useBloom)
        {
        }
        else
        {
        }
    }
    if (m_useBloom)
    {
        ImGui::Separator();
        ImGui::SliderFloat("##BloomThreshold", &m_bloomThreshold, 0.1f, 10.0f, "Bloom Threshold = %.1f");
        ImGui::SliderFloat("##Exposure", &m_exposure, 0.1f, 4.0f, "Exposure = %.2f");
        ImGui::SliderFloat("##Gamma", &m_gamma, 0.1f, 3.0f, "Gamma = %.2f");
    }
}

void BlackHole::OnResize()
{
    CreateFBOs();
    SetProjectionMatrix();
    m_ImGuiFirstTime = true;
}

void BlackHole::SetProjectionMatrix()
{
    glm::mat4 camera_proj = Application::Get().GetCamera().GetProj();
    m_quad.SetProjection(camera_proj, false);
}

void BlackHole::SetGraphicsPreset(const graphicsPreset &preset)
{
    m_diskInnerRadius = preset.innerRadius;
    m_a = preset.a;
    m_Tmax = preset.temp;
    m_diskAbsorption = preset.diskAbsorption;
    m_bloomBackgroundMultiplier = preset.backgroundBrightness;
    m_bloomDiskMultiplier = preset.diskBrightness;
    m_dMdt = preset.dMdt;
    m_blueshiftPower = preset.blueshiftPOW;
    m_brightnessFromDiskVel = preset.diskDopplerBrightness;
    m_bloomThreshold = preset.bloomThreshold;
    m_exposure = preset.exposure;
    m_gamma = preset.gamma;
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
