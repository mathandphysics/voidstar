#include "BlackHole.h"

#include "Application.h"
#include <cmath>



graphicsPreset defaultPreset = { 0.95f, 0.8f, 2.5f, 0.4f, 0.7f, 1.0f, 3.0f, 2.0f, 4400.0f, 0.0f };
graphicsPreset interstellarPreset = { 1.6f, 0.1f, 10.0f, 0.49f, 0.56f, 1.0f, 3.0f, 1.9f, 2800.0f, 385.0f };
graphicsPreset shadowPreset = { 0.7f, 0.1f, 4.8f, 0.43f, 0.19f, 0.0f, 5.2f, 0.0f, 40000.0f, 5000.0f };
graphicsPreset celshadePreset = { 2.0f, 0.1f, 10.0f, 1.40f, 0.10f, 0.0f, 3.0f, 3.0f, 2100.0f, 385.0f };


BlackHole::BlackHole()
{
    CreateScreenQuad();

    // y=1 puts the camera slightly above the xz-plane of the accretion disk.
    Application::Get().GetCamera().SetCameraPos(glm::vec3(0.0f, 1.0f, -30.0f));

    LoadTextures();
    CompileBHShaders();
    CreateFBOs();
    CompilePostShaders();

    SetGraphicsPreset(interstellarPreset);
}

BlackHole::~BlackHole()
{
}

void BlackHole::OnUpdate()
{
    float deltaTime = Application::Get().GetTimer().GetDeltaTime();
    if (m_rotateDisk)
    {
        m_diskRotationAngle -= deltaTime * m_diskRotationSpeed;
    }
}

void BlackHole::Draw()
{
    // Draw to initial off-screen FBO
    m_fbo.Bind();
    m_quad.SetShader(m_selectedShaderString);
    SetShaderUniforms();
    m_quad.Draw();
    m_fbo.Unbind();

    // Post-processing
    PostProcess();

    // Final draw to screen
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
    m_quad.Initialize((const float*)&positions[0], 4, layout, triangles, 6, m_selectedShaderString);
    m_quad.SetPosition(glm::vec3(0.0f, 0.0f, 1.0f / glm::tan(glm::radians(60.0f) / 2.0f)));
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
    std::shared_ptr<Shader> kerr_shader = Renderer::Get().GetShader(m_kerrBlackHoleShaderPath);
    kerr_shader->Bind();
    SetShaderUniforms();

    std::shared_ptr<Shader> relativistic_shader = Renderer::Get().GetShader(m_gravitationalLensingShaderPath);
    relativistic_shader->Bind();
    SetShaderUniforms();

    std::shared_ptr<Shader> nonrelativistic_shader = Renderer::Get().GetShader(m_flatShaderPath);
    nonrelativistic_shader->Bind();
    SetShaderUniforms();

    std::shared_ptr<Shader> flatspace_shader = Renderer::Get().GetShader(m_flatSpacetimeShaderPath);
    flatspace_shader->Bind();
    SetShaderUniforms();
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
    m_pingFBO.SetSpecification(fbospec);
    m_pingFBO.Unbind();
    m_pongFBO.SetSpecification(fbospec);
    m_pongFBO.Unbind();
    fbospec.numColouredAttachments = 2;
    m_fbo.SetSpecification(fbospec);
    m_fbo.Unbind();
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
    shader->SetUniform1f("u_BHMass", m_mass);
    shader->SetUniform1f("u_a", m_a);
    shader->SetUniform1f("u_diskRotationAngle", m_diskRotationAngle);

    shader->SetUniform1i("u_msaa", m_msaa);

    shader->SetUniform1i("u_maxSteps", m_maxSteps);
    shader->SetUniform1f("u_stepSize", m_stepSize);
    shader->SetUniform1f("u_maxDistance", m_maxDistance);
    shader->SetUniform1f("u_epsilon", m_epsilon);

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
    shader->SetUniform1f("u_brightnessFromDistance", m_brightnessFromDistance);
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
    GLCall(glBindTexture(GL_TEXTURE_2D, m_fbo.GetColourAttachments()[0]));
    GLCall(glActiveTexture(GL_TEXTURE0 + m_screenTextureSlot + 1));
    if (m_horizontalPass)
    {
        GLCall(glBindTexture(GL_TEXTURE_2D, m_pongFBO.GetColourAttachments()[0]));
    }
    else
    {
        GLCall(glBindTexture(GL_TEXTURE_2D, m_pingFBO.GetColourAttachments()[0]));
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
                m_pingFBO.Bind();
                GLCall(GLCall(glActiveTexture(GL_TEXTURE0 + m_screenTextureSlot)));
                if (m_firstIteration)
                {
                    GLCall(glBindTexture(GL_TEXTURE_2D, m_fbo.GetColourAttachments()[1]));
                }
                else
                {
                    GLCall(glBindTexture(GL_TEXTURE_2D, m_pongFBO.GetColourAttachments()[0]));
                }
            }
            else
            {
                m_pongFBO.Bind();
                GLCall(GLCall(glActiveTexture(GL_TEXTURE0 + m_screenTextureSlot)));
                if (m_firstIteration)
                {
                    GLCall(glBindTexture(GL_TEXTURE_2D, m_fbo.GetColourAttachments()[1]));
                }
                else
                {
                    GLCall(glBindTexture(GL_TEXTURE_2D, m_pingFBO.GetColourAttachments()[0]));
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

float BlackHole::CalculateISCO(float m, float a)
{
    // Calculate in the innermost stable circular orbit for a black hole with parameters mass m and rotation a.
    // Result is the theoretically smallest inner radius for a stable accretion disk.
    // Assumes G = c = 1
    if (m == 0)
    {
        // In this case, there is no stable orbit because there is no mass.  So the result is actually infinity.
        return 0.0f;
    }
    float rs = 2.0f * m;
    float chi = 2.0f * a / rs;
    float z1 = 1 + std::powf(1 - chi*chi, 1.0f/3.0f) * (std::powf(1 + chi, 1.0f/3.0f) + std::powf(1 - chi, 1.0f/3.0f));
    float z2 = std::powf(3.0f*chi*chi + z1*z1, 1.0f / 2.0f);
    return m * (3 + z2 - std::powf((3-z1)*(3+z1+2*z2), 1.0f / 2.0f));
}

void BlackHole::OnImGuiRender()
{
    // Fix the controls window to the right side of the screen.
    ImVec2 work_size = ImGui::GetMainViewport()->WorkSize;
    ImVec2 window_size = ImVec2(310, work_size.y);
    ImVec2 window_pos = ImGui::GetMainViewport()->WorkPos;
    window_pos[0] += work_size.x - window_size[0];
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Once);

    ImGuiWindowFlags imgui_window_flags = ImGuiWindowFlags_NoCollapse;
    imgui_window_flags |= ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;
    imgui_window_flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus;
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

            ImGuiBHProperties();
            ImGui::Separator();
            ImGui::Separator();

            ImGuiSimQuality();
            ImGui::Separator();
            ImGui::Separator();
            
            ImGuiChooseBH();
            ImGui::Separator();
            ImGui::Separator();

            ImGuiDebug();
            ImGui::Separator();
            ImGui::Separator();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lighting/Colour"))
        {

            ImGuiCinematic();
            ImGui::Separator();
            ImGui::Separator();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
        ImGui::PopItemWidth();
    }
    ImGui::End();
}

void BlackHole::ImGuiRenderStats()
{
    ImGui::Text("Render statistics:");
    Application::Get().GetMenu().PrintRenderStatistics();
    if (ImGui::Checkbox("Vsync", &m_vsync))
    {
        glfwSwapInterval((int)m_vsync);
    }
    ImGui::Text("Anti-Aliasing: WARNING!");
    ImGui::SameLine();
    HelpMarker("MSAA is a very expensive operation for a ray marcher.  As a rule of thumb, if your uncapped framerate at "
                "MSAA=1 is F, then your framerate at MSAA=x will be roughly F/(x*x).  So, for example, going from MSAA=1 "
                "to MSAA=3 will reduce framerate by a factor of 9.");
    ImGui::SliderInt("##MSAA", &m_msaa, 1, 4, "MSAA = %d");
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
    if (ImGui::SliderFloat("##BlackHoleMass", &m_mass, 0.0, 3.0, "Black Hole Mass = %.1f"))
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
        float isco = CalculateISCO(m_mass, m_a);
        m_diskInnerRadius = isco;
    }
    ImGui::SliderFloat("##InnerDiskRadius", &m_diskInnerRadius, 1.5f * m_radius, 5.0f * m_radius, "Inner Disk Radius = %.1f");
    ImGui::SliderFloat("##OuterDiskRadius", &m_diskOuterRadius, m_diskInnerRadius, 10.0f * m_radius,
        "Outer Disk Radius = %.1f");
    if (ImGui::SliderFloat("##a", &m_a, 0.0f, m_mass - 0.05f, "a = %.2f"))
    {
        float isco = CalculateISCO(m_mass, m_a);
        m_diskInnerRadius = isco;
    }
    ImGui::SliderFloat("##DiskAbsorption", &m_diskAbsorption, 0.0f, 7.0f, "Disk Absorption = %.1f");
    ImGui::Checkbox("Rotate Disk", &m_rotateDisk);
    if (m_rotateDisk)
    {
        ImGui::SliderFloat("##DiskRotationSpeed", &m_diskRotationSpeed, 0.0f, 0.3f, "Rotation Speed = %.3f");
    }
}

void BlackHole::ImGuiSimQuality()
{
    ImGui::Text("Simulation Quality:");
    ImGui::SameLine();
    HelpMarker("Making Step Size smaller will increase the accuracy of the simulation.  However, it "
        "can also make the ring or black hole disappear.  If this happens, increase Max Steps to "
        "make everything reappear with greater accuracy.  WARNING: this will significantly increase "
        "computational cost and slow down the simulation.");
    ImGui::SliderFloat("##StepSize", &m_stepSize, 0.01f, 0.5f, "Step Size = %.2f");
    ImGui::SliderInt("##MaxSteps", &m_maxSteps, 1, 10000, "Max Steps = %d", 0);
    //ImGui::SliderFloat("##MaxDistance", &m_maxDistance, 1.0f, 10000.0, "Max Distance = %.0f");
    //ImGui::SliderFloat("##Epsilon", &m_epsilon, 0.00001f, 0.001f, "Epsilon = %.5f");
}

void BlackHole::ImGuiChooseBH()
{
    ImGui::Text("Black Hole Selector:");
    if (ImGui::RadioButton("Kerr BH", &m_shaderSelector, 0))
    {
        m_selectedShaderString = m_kerrBlackHoleShaderPath;
        m_maxSteps = 200;
    }
    ImGui::SameLine();
    HelpMarker("Accurate rotating black hole using the Kerr metric of General Relativity.  The parameter \"a\" above is the spin parameter.  a=0 is a non-rotating black hole.");
    if (ImGui::RadioButton("Classical BH (Fast)", &m_shaderSelector, 1))
    {
        m_selectedShaderString = m_gravitationalLensingShaderPath;
        m_maxSteps = 2000;
    }
    ImGui::SameLine();
    HelpMarker("Inaccurate, non-rotating black hole based on Newtonian Gravity.");
    if (ImGui::RadioButton("Minkowski BH", &m_shaderSelector, 2))
    {
        m_selectedShaderString = m_flatSpacetimeShaderPath;
        m_maxSteps = 200;
    }
    ImGui::SameLine();
    HelpMarker("Minkowski metric of General Relativity.  Equivalent to (but slower than) Flat BH (Fast).");
    if (ImGui::RadioButton("Flat BH (Fast)", &m_shaderSelector, 3))
    {
        m_selectedShaderString = m_flatShaderPath;
    }
    ImGui::SameLine();
    HelpMarker("Typical ray tracer using straight line rays/intersections.");
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
        if (ImGui::RadioButton("Cel-Shaded", &m_presetSelector, 2))
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
        ImGui::SliderFloat("##Exposure", &m_exposure, 0.1f, 2.0f, "Exposure = %.2f");
        ImGui::SliderFloat("##Gamma", &m_gamma, 0.1f, 3.0f, "Gamma = %.2f");
        ImGui::SliderFloat("##BrightnessFromDistance", &m_brightnessFromDistance, 0.0f, 10.0f, "Brightness from Distance = %.2f");
        ImGui::SliderFloat("##BrightnessFromDiskVel", &m_brightnessFromDiskVel, 0.0f, 10.0f, "Disk Velocity Brightness = %.1f");
        ImGui::SliderFloat("##ColourshiftPower", &m_colourshiftPower, 0.0f, 10.0f, "Disk Colour Power = %.1f");
        ImGui::SliderFloat("##ColourshiftMultiplier", &m_colourshiftMultiplier, 100.0f, 40000.0f, "Disk Colour Multiplier = %.0f");
        ImGui::SliderFloat("##ColourshiftOffset", &m_colourshiftOffset, 0.0f, 5000.0f, "Disk Colour Offset = %.0f");

    }
}

void BlackHole::SetGraphicsPreset(graphicsPreset preset)
{
    m_bloomThreshold = preset.bloomThreshold;
    m_bloomBackgroundMultiplier = preset.bloomBackgroundMultiplier;
    m_bloomDiskMultiplier = preset.bloomDiskMultiplier;
    m_exposure = preset.exposure;
    m_gamma = preset.gamma;
    m_brightnessFromDistance = preset.brightnessFromDistance;
    m_brightnessFromDiskVel = preset.brightnessFromDiskVel;
    m_colourshiftPower = preset.colourshiftPower;
    m_colourshiftMultiplier = preset.colourshiftMultiplier;
    m_colourshiftOffset = preset.colourshiftOffset;
}
