#include "Shader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Renderer.h"

Shader::Shader()
{
}

Shader::Shader(const std::string& filepath)
{
    m_FilePath = filepath;
    ShaderProgramSource source = ParseShader(filepath);
    m_RendererID = CreateShaderProgram(source.VertexSource, source.FragmentSource);
}

Shader::Shader(const std::string& filepath, const std::vector<std::string>& vertexDefines, const std::vector<std::string>& fragmentDefines)
{
    m_FilePath = filepath;
    ShaderProgramSource source = ParseShader(filepath);
    InsertDefines(source.VertexSource, vertexDefines);
    InsertDefines(source.FragmentSource, fragmentDefines);

    m_RendererID = CreateShaderProgram(source.VertexSource, source.FragmentSource);
}

Shader::~Shader()
{
#ifndef NDEBUG
    std::cout << "Deleting shader with ID: " << m_RendererID << std::endl;
#endif
    GLCall(glDeleteProgram(m_RendererID));
}

ShaderProgramSource Shader::ParseShader(const std::string& filepath)
{
    std::ifstream stream(filepath);

    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;
    while (getline(stream, line))
    {
        if (line.find("#shader") != std::string::npos)
        {
            if (line.find("vertex") != std::string::npos)
            {
                type = ShaderType::VERTEX;
            }

            else if (line.find("fragment") != std::string::npos)
            {
                type = ShaderType::FRAGMENT;
            }
        }

        else
        {
            if (type != ShaderType::NONE)
            {
                ss[(int)type] << line << '\n';
            }
        }
    }
    return { ss[0].str(), ss[1].str() };
}

void Shader::InsertDefines(std::string& shadersource, const std::vector<std::string>& defines)
{

    // Go through the source and insert the defines lines.
    // Find the #version declaration.  Defines must always go after this per the GLSL standard.
    size_t found = shadersource.find("#version", 0);
    size_t insertloc;
    if (found != std::string::npos)
    {
        // Now find the next newline.
        insertloc = shadersource.find("\n", found) + 1;
    }
    else
    {
        // Then there's no #version declaration and we can just insert at the beginning of the file.
        insertloc = 0;
    }
    for (auto& defStatement : defines)
    {
        if (!defStatement.empty())
        {
            m_FilePath.append(defStatement);
            shadersource.insert(insertloc, "\n");
            shadersource.insert(insertloc, defStatement);
            shadersource.insert(insertloc, "#define ");
        }
    }
}

unsigned int Shader::CompileShader(const std::string& source, unsigned int type)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    GLCall(glShaderSource(id, 1, &src, nullptr));
    GLCall(glCompileShader(id));

    // Error checking
    int result;
    GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));
    if (result == GL_FALSE) {
        int length;
        GLCall(glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length));
        char* message = new char[length];
        glGetShaderInfoLog(id, length, &length, message);
        std::cout << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader!" << std::endl;
        std::cout << message << std::endl;
        delete[] message;
        GLCall(glDeleteShader(id));
        return 0;
    }

    return id;
}

unsigned int Shader::CreateShaderProgram(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(vertexShader, GL_VERTEX_SHADER);
    unsigned int fs = CompileShader(fragmentShader, GL_FRAGMENT_SHADER);

    GLCall(glAttachShader(program, vs));
    GLCall(glAttachShader(program, fs));

    GLCall(glLinkProgram(program));
#ifndef NDEBUG
    GLCall(glValidateProgram(program));
#endif

    GLCall(glDetachShader(program, vs));
    GLCall(glDetachShader(program, fs));
    GLCall(glDeleteShader(vs));
    GLCall(glDeleteShader(fs));

#ifndef NDEBUG
    std::cout << "Created shader program: " << m_FilePath << ", with RendererID: " << program << std::endl;
#endif

    return program;
}


void Shader::Bind() const
{
    GLCall(glUseProgram(m_RendererID));
}

void Shader::Unbind() const
{
    GLCall(glUseProgram(0));
}

void Shader::SetUniform1i(const std::string& name, int value)
{
    GLCall(glUniform1i(GetUniformLocation(name), value));
}

void Shader::SetUniform1f(const std::string& name, float value)
{
    GLCall(glUniform1f(GetUniformLocation(name), value));
}

void Shader::SetUniform3f(const std::string& name, float v0, float v1, float v2)
{
    GLCall(glUniform3f(GetUniformLocation(name), v0, v1, v2));
}

void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    GLCall(glUniform4f(GetUniformLocation(name) , v0, v1, v2, v3));
}

void Shader::SetUniformMat4f(const std::string& name, const glm::mat4& matrix)
{
    GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

int Shader::GetUniformLocation(const std::string& name)
{
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
        return m_UniformLocationCache[name];

    else
    {
        GLCall(int location = glGetUniformLocation(m_RendererID, name.c_str()));
        if (location == -1)
            std::cout << "Warning: uniform " << name << " doesn't exist!" << std::endl;

        m_UniformLocationCache[name] = location;
        return location;
    }
}
