#pragma once

#include "Texture.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Shader.h"

#include <memory>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"



struct Vertex
{
	glm::vec3 position;
	glm::vec4 colour;
	glm::vec2 texCoords;
	float texIndex; // To use different texture slots.
};


struct MeshVertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
	float texIndex; // To use different texture slots.
};


class Mesh
{
public:
	Mesh();
	Mesh(const float* vertData, unsigned int numVerts, VertexBufferLayout layout,
		const unsigned int* indexBuffer, unsigned int numIndices, const std::string& shaderPath,
		const std::string& texturePath = "", bool flip = true);
	~Mesh();

	void Initialize(const float* vertData, unsigned int numVerts, VertexBufferLayout layout,
		const unsigned int* indexBuffer, unsigned int numIndices, const std::string& shaderPath,
		const std::string& texturePath = "", bool flip = true);
	void SetTexture(const std::string& path, bool flip);
	void SetShader(const std::string& path);
	void SetPosition(glm::vec3 position);
	void SetRotation(glm::mat4 rotation);
	void SetProjection(glm::mat4 proj, bool useOrtho = false);
	void SetWireFrame(bool useWireFrame);

	glm::vec3 GetPosition() { return m_Position; }

	std::shared_ptr<Shader> GetShader();

	void Draw();

private:
	glm::mat4 getMVP();
	void ActivateTexture();
	void DeactivateTexture();
	void ActivateWireFrame();
	void DeactivateWireFrame();

	unsigned int m_nVerts = 0;
	unsigned int m_nIndices = 0;

	bool m_useWireFrame = false;
	bool m_useTexture = false;
	std::string m_texturePath;
	bool m_flipTexture = true;
	std::shared_ptr<Texture2D> m_texture;

	glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::mat4 m_Rotation = glm::mat4(1.0f);
	glm::mat4 m_Model = glm::mat4(1.0f);
	glm::mat4 m_View = glm::mat4(1.0f);
	glm::mat4 m_ViewInv = glm::inverse(m_View);
	glm::mat4 m_Proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.0001f, 1000.0f);
	glm::mat4 m_ProjInv = glm::inverse(m_ProjInv);
	bool m_Orthographic = false;

	std::shared_ptr<VertexArray> m_vao;
	std::shared_ptr<VertexBuffer> m_vbo;
	std::shared_ptr<IndexBuffer> m_ibo;
	std::shared_ptr<Shader> m_shader;
};