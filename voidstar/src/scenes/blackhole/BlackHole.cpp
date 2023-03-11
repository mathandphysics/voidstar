#include "BlackHole.h"

#include "Application.h"

#include "GLFW/glfw3.h"

BlackHole::BlackHole()
{
    // Make a rectangle that exactly fills the screen.  Then just use the fragment shader to draw on it.
    glm::vec3 v1 = glm::vec3(-1.0f, -1.0f, 0.0f);
    glm::vec3 v2 = glm::vec3(1.0f, -1.0f, 0.0f);
    glm::vec3 v3 = glm::vec3(1.0f, 1.0f, 0.0f);
    glm::vec3 v4 = glm::vec3(-1.0f, 1.0f, 0.0f);

    std::vector<glm::vec3> positions({ v1, v2, v3, v4 });

    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    VertexBufferLayout layout;
    layout.Push<float>(3);
    // Load and compile both shaders.
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

    std::shared_ptr<Shader> relativistic_shader = Renderer::Get().GetShader(m_gravitationalLensingShaderPath);
    relativistic_shader->Bind();
    relativistic_shader->SetUniform1i("diskTexture", m_diskTextureSlot);
    relativistic_shader->SetUniform1i("skybox", m_skyboxTextureSlot);
    SetShaderUniforms();

    std::shared_ptr<Shader> nonrelativistic_shader = Renderer::Get().GetShader(m_flatShaderPath);
    nonrelativistic_shader->Bind();
    nonrelativistic_shader->SetUniform1i("diskTexture", m_diskTextureSlot);
    nonrelativistic_shader->SetUniform1i("skybox", m_skyboxTextureSlot);
    SetShaderUniforms();
}

BlackHole::~BlackHole()
{
}

void BlackHole::OnUpdate()
{
    float deltaTime = Application::Get().GetTimer().GetDeltaTime();
    if (m_rotateDisk)
    {
        m_diskRotationAngle += deltaTime * m_diskRotationSpeed;
    }
}

void BlackHole::Draw()
{
    SetShaderUniforms();
    m_quad.Draw();
    m_cubemap.Unbind();
    m_diskTexture->Unbind();
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
    shader->SetUniform1f("u_diskRotationAngle", m_diskRotationAngle);

    shader->SetUniform1i("u_useDebugSphereTexture", (int)m_useDebugSphereTexture);
    shader->SetUniform1i("u_useDebugDiskTexture", (int)m_useDebugDiskTexture);
    shader->SetUniform3f("u_sphereDebugColour1", m_sphereDebugColour1[0], m_sphereDebugColour1[1], m_sphereDebugColour1[2]);
    shader->SetUniform3f("u_sphereDebugColour2", m_sphereDebugColour2[0], m_sphereDebugColour2[1], m_sphereDebugColour2[2]);
    shader->SetUniform3f("u_diskDebugColourTop1", m_diskDebugColourTop1[0], m_diskDebugColourTop1[1], m_diskDebugColourTop1[2]);
    shader->SetUniform3f("u_diskDebugColourTop2", m_diskDebugColourTop2[0], m_diskDebugColourTop2[1], m_diskDebugColourTop2[2]);
    shader->SetUniform3f("u_diskDebugColourBottom1", m_diskDebugColourBottom1[0], m_diskDebugColourBottom1[1], m_diskDebugColourBottom1[2]);
    shader->SetUniform3f("u_diskDebugColourBottom2", m_diskDebugColourBottom2[0], m_diskDebugColourBottom2[1], m_diskDebugColourBottom2[2]);

    glm::vec3 cameraPos = Application::Get().GetCamera().GetPosition();
    shader->SetUniform3f("u_cameraPos", cameraPos.x, cameraPos.y, cameraPos.z);

    m_diskTexture->Bind(m_diskTextureSlot);
    m_cubemap.Bind(m_skyboxTextureSlot);
}

void BlackHole::OnImGuiRender()
{
    ImGuiWindowFlags imgui_window_flags = ImGuiWindowFlags_NoCollapse;
    imgui_window_flags |= ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;
    ImGui::Begin("Black Hole Controls", nullptr, imgui_window_flags);
    ImGui::PushItemWidth(-FLT_MIN);
    if (ImGui::SliderFloat("##BlackHoleMass", &m_mass, 0.0, 3.0, "Black Hole Mass = %.1f"))
    {
        m_radius = 2.0f * m_mass;
        m_diskInnerRadius = 3.0f * m_radius;
    }
    ImGui::SliderFloat("##InnerDiskRadius", &m_diskInnerRadius, 1.5f * m_radius, 5.0f * m_radius, "Inner Disk Radius = %.1f");
    ImGui::SliderFloat("##OuterDiskRadius", &m_diskOuterRadius, m_diskInnerRadius, 10.0f * m_radius,
        "Outer Disk Radius = %.1f");
    ImGui::PopItemWidth();
    ImGui::Separator();
    ImGui::Separator();
    if (ImGui::Checkbox("Gravitational Lensing", &m_drawLensing))
    {
        if (m_drawLensing)
        {
            m_quad.SetShader(m_gravitationalLensingShaderPath);
        }
        else
        {
            m_quad.SetShader(m_flatShaderPath);
        }
    }
    ImGui::Checkbox("Rotate Disk", &m_rotateDisk);
    if (m_rotateDisk)
    {
        ImGui::PushItemWidth(-FLT_MIN);
        ImGui::SliderFloat("##DiskRotationSpeed", &m_diskRotationSpeed, -0.3f, 0.3f, "Rotation Speed = %.3f");
        ImGui::PopItemWidth();
    }

    ImGui::Checkbox("Disk Debug Texture", &m_useDebugDiskTexture);
    ImGui::Checkbox("Sphere Debug Texture", &m_useDebugSphereTexture);

    ImGui::Separator();
    ImGui::Separator();

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
    ImGui::End();
}
