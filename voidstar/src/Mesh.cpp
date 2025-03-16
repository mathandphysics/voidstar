#include "Mesh.h"

#include "Application.h"


Mesh::Mesh()
{
}

Mesh::Mesh(const float* vertData, unsigned int numVerts, VertexBufferLayout layout,
	const unsigned int* indexBuffer, unsigned int numIndices, const std::string& shaderPath,
	const std::string& texturePath, bool flip)
{

	Initialize(vertData, numVerts, layout, indexBuffer, numIndices, shaderPath, texturePath, flip);
}

Mesh::~Mesh()
{
}

void Mesh::Initialize(const float* vertData, unsigned int numVerts, VertexBufferLayout layout,
	const unsigned int* indexBuffer, unsigned int numIndices, const std::string& shaderPath,
	const std::string& texturePath, bool flip, const std::vector<std::string>& vertexDefines, 
	const std::vector<std::string>& fragmentDefines)
{
	if (!texturePath.empty())
	{
		SetTexture(texturePath, flip);
	}

	m_nVerts = numVerts;
	m_nIndices = numIndices;

	m_vbo = std::make_shared<VertexBuffer>(&vertData[0], m_nVerts * layout.GetStride());
	m_vao = std::make_shared<VertexArray>();
	m_vao->AddBuffer(*m_vbo, layout);

	m_ibo = std::make_shared<IndexBuffer>(indexBuffer, numIndices);

	SetShader(shaderPath, vertexDefines, fragmentDefines);
}

void Mesh::SetTexture(const std::string& path, bool flip)
{
	m_texturePath = path;
	m_useTexture = true;
	m_flipTexture = flip;
	m_texture = Renderer::Get().GetTexture(path, flip);
}

void Mesh::SetShader(const std::string& path)
{
	m_shader = Renderer::Get().GetShader(path);
	m_shader->Bind();
	m_shader->SetUniformMat4f("u_MVP", m_Proj);
	m_shader->Unbind();
}

void Mesh::SetShader(const std::string& path, const std::vector<std::string>& vertexDefines, const std::vector<std::string>& fragmentDefines)
{
	m_shader = Renderer::Get().GetShader(path, vertexDefines, fragmentDefines);
	m_shader->Bind();
	m_shader->SetUniformMat4f("u_MVP", m_Proj);
	m_shader->Unbind();
}

void Mesh::SetPosition(glm::vec3 position)
{
	m_Position = position;
}

void Mesh::SetRotation(glm::mat4 rotation)
{
	m_Rotation = rotation;
}

void Mesh::SetProjection(glm::mat4 proj, bool useOrtho)
{
	m_Orthographic = useOrtho;
	m_Proj = proj;
	m_ProjInv = glm::inverse(m_Proj);
}

void Mesh::SetWireFrame(bool useWireFrame)
{
	m_useWireFrame = useWireFrame;
}

std::shared_ptr<Shader> Mesh::GetShader()
{
	return m_shader;
}

void Mesh::Draw()
{
	Renderer& renderer = Renderer::Get();

	m_shader->Bind();
	glm::mat4 mvp = getMVP();
	m_shader->SetUniformMat4f("u_MVP", mvp);
	m_shader->SetUniformMat4f("u_Model", m_Model);
	m_shader->SetUniformMat4f("u_View", m_View);
	m_shader->SetUniformMat4f("u_ViewInv", m_ViewInv);
	m_shader->SetUniformMat4f("u_Proj", m_Proj);
	m_shader->SetUniformMat4f("u_ProjInv", m_ProjInv);
	glm::vec3 camera_pos = Application::Get().GetCamera().GetPosition();
	m_shader->SetUniform3f("u_CameraPos", camera_pos);

	ActivateTexture();
	ActivateWireFrame();

	renderer.Draw(*m_vao, *m_ibo, *m_shader);

	DeactivateWireFrame();
	DeactivateTexture();

	m_shader->Unbind();
}

glm::mat4 Mesh::getMVP()
{
	glm::mat4 mvp;
	m_Model = glm::translate(glm::mat4(1.0f), m_Position) * m_Rotation;
	if (m_Orthographic) {
		mvp = m_Proj * m_Model;
	}
	else {
		m_View = Application::Get().GetCamera().GetView();
		m_ViewInv = glm::inverse(m_View);
		mvp = m_Proj * m_View * m_Model;
	}
	return mvp;
}

void Mesh::ActivateTexture()
{
	if (m_useTexture)
	{
		m_texture->Bind();
	}
}

void Mesh::DeactivateTexture()
{
	if (m_useTexture)
	{
		m_texture->Unbind();
	}
}

void Mesh::ActivateWireFrame()
{
	if (m_useWireFrame)
	{
		GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
	}
}

void Mesh::DeactivateWireFrame()
{
	if (m_useWireFrame)
	{
		GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
	}
}

