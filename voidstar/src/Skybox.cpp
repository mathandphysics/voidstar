#include "Skybox.h"
#include "Application.h"

#include <glm/gtc/matrix_transform.hpp>


Skybox::Skybox()
{
}

Skybox::Skybox(std::vector<std::string> paths, const std::string& shaderPath)
{
    SetSkybox(paths, shaderPath);
}

Skybox::~Skybox()
{
}

void Skybox::SetSkybox(std::vector<std::string> paths, const std::string& shaderPath)
{
    m_paths = paths;
    m_cubeMap.SetCubeMap(paths);

    float skyboxVerts[] = {
        // Positions
        // z-neg face
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        // x-neg face
        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        // x-pos face
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

         // z-pos face
        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        // y-pos face
        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        // y-neg face
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    m_vbo = std::make_shared<VertexBuffer>(skyboxVerts, 36 * 3 * (unsigned int)sizeof(float));

    m_shader = Renderer::Get().GetShader(shaderPath);
    VertexBufferLayout layout;
    layout.Push<float>(3);
    m_vao = std::make_shared<VertexArray>();
    m_vao->AddBuffer(*m_vbo, layout);
}

void Skybox::Draw()
{
	glDepthMask(GL_FALSE);

	m_shader->Bind();
	m_cubeMap.Bind();
	m_vao->Bind();

    glm::mat4 current_camera_view = Application::Get().GetCamera().GetSkyboxView();
    glm::mat4 mvp = m_proj * current_camera_view;
    m_shader->SetUniformMat4f("u_MVP", mvp);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	m_vao->Unbind();
	m_cubeMap.Unbind();
	m_shader->Unbind();

	glDepthMask(GL_TRUE);
}
