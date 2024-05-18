#include "BlackHole.h"

#include "Application.h"

BlackHole::BlackHole()
{
    struct mesh_with_tc {
        glm::vec3 pos;
        glm::vec2 tc;
    };
    // Make a rectangle that exactly fills the screen.  Then just use the fragment shader to draw on it.
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

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    VertexBufferLayout layout;
    layout.Push<float>(3);
    layout.Push<float>(2);
    // Load and compile shaders.
    Renderer::Get().GetShader(m_flatShaderPath);
    Renderer::Get().GetShader(m_gravitationalLensingShaderPath);
    if (m_drawLensing)
    {
        m_quad = QuadColoured((const float*)&positions[0], layout, m_gravitationalLensingShaderPath);
    }
    else
    {
        m_quad = QuadColoured((const float*)&positions[0], layout, m_flatShaderPath);
    }
    m_quad.SetPosition(glm::vec3(0.0f, 0.0f, 1.0f / glm::tan(glm::radians(60.0f) / 2.0f)));
    m_quad.SetProjection(m_proj, false);

    Application::Get().GetCamera().SetCameraPos(glm::vec3(0.0f, 1.0f, -30.0f));

    // Ordering of faces: xpos, xneg, ypos, yneg, zpos, zneg.
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

    /*
    if (m_drawLensing)
    {
        //m_quad.SetShader(m_gravitationalLensingShaderPath);
        m_quad.SetShader(m_kerrBlackHoleShaderPath);
        //m_quad.SetShader(m_flatSpacetimeShaderPath);
    }
    else
    {
        m_quad.SetShader(m_flatSpacetimeShaderPath);
        //m_quad.SetShader(m_flatShaderPath);
    }
    */
    m_quad.SetShader(m_selectedShaderString);
    SetShaderUniforms();
    m_quad.Draw();

    m_fbo.Unbind();

    // Post-processing
    PostProcess();

    // Draw to screen
    m_quad.SetShader(m_BloomShaderPath);
    SetScreenShaderUniforms();
    m_quad.Draw();
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

    shader->SetUniform1i("u_maxSteps", m_maxSteps);
    shader->SetUniform1f("u_stepSize", m_stepSize);
    shader->SetUniform1f("u_maxDistance", m_maxDistance);
    shader->SetUniform1f("u_epsilon", m_epsilon);

    shader->SetUniform1i("diskTexture", m_diskTextureSlot);
    shader->SetUniform1i("skybox", m_skyboxTextureSlot);
    shader->SetUniform1i("sphereTexture", m_sphereTextureSlot);
    shader->SetUniform1i("u_useSphereTexture", m_useSphereTexture);
    shader->SetUniform1i("u_useDebugSphereTexture", (int)m_useDebugSphereTexture);
    shader->SetUniform1i("u_useDebugDiskTexture", (int)m_useDebugDiskTexture);
    shader->SetUniform3f("u_sphereDebugColour1", m_sphereDebugColour1[0], m_sphereDebugColour1[1], m_sphereDebugColour1[2]);
    shader->SetUniform3f("u_sphereDebugColour2", m_sphereDebugColour2[0], m_sphereDebugColour2[1], m_sphereDebugColour2[2]);
    shader->SetUniform3f("u_diskDebugColourTop1", m_diskDebugColourTop1[0], m_diskDebugColourTop1[1], m_diskDebugColourTop1[2]);
    shader->SetUniform3f("u_diskDebugColourTop2", m_diskDebugColourTop2[0], m_diskDebugColourTop2[1], m_diskDebugColourTop2[2]);
    shader->SetUniform3f("u_diskDebugColourBottom1", m_diskDebugColourBottom1[0], m_diskDebugColourBottom1[1], m_diskDebugColourBottom1[2]);
    shader->SetUniform3f("u_diskDebugColourBottom2", m_diskDebugColourBottom2[0], m_diskDebugColourBottom2[1], m_diskDebugColourBottom2[2]);

    shader->SetUniform1i("u_bloom", m_useBloom);
    shader->SetUniform1f("u_bloomThreshold", m_bloomThreshold);
    shader->SetUniform1f("u_bloomBackgroundMultiplier", m_bloomBackgroundMultiplier);
    shader->SetUniform1f("u_bloomDiskMultiplier", m_bloomDiskMultiplier);
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

void BlackHole::OnImGuiRender()
{
    ImGuiWindowFlags imgui_window_flags = ImGuiWindowFlags_NoCollapse;
    imgui_window_flags |= ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;
    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    ImGui::Begin("Black Hole Controls", nullptr, imgui_window_flags);
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::BeginTabBar("##BHTabBar", tab_bar_flags))
    {
        if (ImGui::BeginTabItem("Simulation"))
        {
            ImGui::Text("Render statistics:");
            Application::Get().GetMenu().PrintRenderStatistics();
            if (ImGui::Checkbox("Vsync", &m_vsync))
            {
                glfwSwapInterval((int)m_vsync);
            }
            ImGui::Separator();
            ImGui::Separator();
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
            }
            ImGui::SliderFloat("##InnerDiskRadius", &m_diskInnerRadius, 1.5f * m_radius, 5.0f * m_radius, "Inner Disk Radius = %.1f");
            ImGui::SliderFloat("##OuterDiskRadius", &m_diskOuterRadius, m_diskInnerRadius, 10.0f * m_radius,
                "Outer Disk Radius = %.1f");
            ImGui::SliderFloat("##a", &m_a, 0.0f, 1.0f, "a = %.2f");
            ImGui::Separator();
            ImGui::Separator();
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
            ImGui::Separator();
            ImGui::Separator();
            ImGui::Text("Black Hole Shader Selector:");
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

            ImGui::Checkbox("Rotate Disk", &m_rotateDisk);
            if (m_rotateDisk)
            {
                ImGui::SliderFloat("##DiskRotationSpeed", &m_diskRotationSpeed, 0.0f, 0.3f, "Rotation Speed = %.3f");
            }

            ImGui::Separator();
            ImGui::Separator();
            ImGui::Text("Debug Properties:");
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

            ImGui::Separator();
            ImGui::Separator();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Lighting/Colour"))
        {
            if (ImGui::Checkbox("Cinematic Mode", &m_useBloom))
            {
                m_bloomBackgroundMultiplier = 0.1f;
            }
            if (m_useBloom)
            {
                ImGui::Separator();
                ImGui::Text("Graphics Presets:");
                if (ImGui::RadioButton("Interstellar-esque", &m_presetSelector, 0))
                {
                    m_bloomThreshold = 0.95f;
                    m_bloomBackgroundMultiplier = 0.1f;
                    m_bloomDiskMultiplier = 2.5f;
                    m_exposure = 0.4f;
                    m_gamma = 0.7f;
                }
                if (ImGui::RadioButton("Dark Shadow", &m_presetSelector, 1))
                {
                    m_bloomThreshold = 0.8f;
                    m_bloomBackgroundMultiplier = 0.1f;
                    m_bloomDiskMultiplier = 3.1f;
                    m_exposure = 0.2f;
                    m_gamma = 0.2f;
                }
                ImGui::Separator();
                ImGui::SliderFloat("##BloomThreshold", &m_bloomThreshold, 0.1f, 10.0f, "Bloom Threshold = %.1f");
                ImGui::SliderFloat("##BackgroundMultiplier", &m_bloomBackgroundMultiplier, 0.1f, 10.0f, "Background = %.1f");
                ImGui::SliderFloat("##DiskMultiplier", &m_bloomDiskMultiplier, 0.1f, 10.0f, "Disk = %.1f");
                ImGui::SliderFloat("##Exposure", &m_exposure, 0.1f, 2.0f, "Exposure = %.2f");
                ImGui::SliderFloat("##Gamma", &m_gamma, 0.1f, 3.0f, "Gamma = %.2f");
            }

            ImGui::Separator();
            ImGui::Separator();


            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
        ImGui::PopItemWidth();
    }
    ImGui::End();
}
