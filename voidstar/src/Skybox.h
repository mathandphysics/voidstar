#pragma once

#include "VertexArray.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"
#include "Shader.h"
#include "Texture.h"

#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Skybox
{
public:
	Skybox();
	Skybox(std::vector<std::string> paths, const std::string& shaderPath);
	~Skybox();

	void SetSkybox(std::vector<std::string> paths, const std::string& shaderPath);

	void Draw();

private:
	std::vector<std::string> m_paths;
	TextureCubeMap m_cubeMap;
	std::shared_ptr<VertexArray> m_vao;
	std::shared_ptr<VertexBuffer> m_vbo;
	std::shared_ptr<Shader> m_shader;
	glm::mat4 m_proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
};