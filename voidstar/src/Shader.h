#pragma once

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragmentSource;
};

class Shader
{
private:
	std::string m_FilePath = "";
	unsigned int m_RendererID = 0;
	std::unordered_map<std::string, int> m_UniformLocationCache;

public:
	Shader();
	Shader(const std::string& filepath);
	Shader(const std::string& filepath, const std::vector<std::string>& vertexDefines, const std::vector<std::string>& fragmentDefines);
	~Shader();

	void Bind() const;
	void Unbind() const;

	const std::string& GetShaderPath() { return m_FilePath; }

	void SetUniform1i(const std::string& name, int value);
	void SetUniform1f(const std::string& name, float value);
	void SetUniform3f(const std::string& name, glm::vec3 v);
	void SetUniform3f(const std::string& name, float v0, float v1, float v2);
	void SetUniform4f(const std::string& name, glm::vec4 v);
	void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void SetUniformMat4f(const std::string& name, const glm::mat4& matrix);

private:
	ShaderProgramSource ParseShader(const std::string& filepath);
	void InsertDefines(std::string& shadersource, const std::vector<std::string>& defines);
	unsigned int CompileShader(const std::string& source, unsigned int type);
	unsigned int CreateShaderProgram(const std::string& vertexShader, const std::string& fragmentShader);
	int GetUniformLocation(const std::string& name);
};