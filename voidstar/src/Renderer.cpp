#include "Renderer.h"

#include <iostream>

void GLClearError()
{
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR)
        std::cout << "OpenGL Error: (" << error << ")" << std::endl;
}

bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError())
    {
        std::cout << "OpenGL Error: (" << error << "): " << function << " " << file << ":" << line << std::endl;
        return false;
    }
    return true;
}

void APIENTRY GLDebugOutput(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

void EnableOpenGLDebugging()
{
    GLCall(glEnable(GL_DEBUG_OUTPUT));
    GLCall(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS));
    GLCall(glDebugMessageCallback(GLDebugOutput, nullptr));
}


Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Clear() const
{
    GLCall(glClear(GL_COLOR_BUFFER_BIT));
}

void Renderer::ClearWithDepth() const
{
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void Renderer::EnableDepth() const
{
    GLCall(glEnable(GL_DEPTH_TEST));
}

void Renderer::DisableDepth() const
{
    GLCall(glDisable(GL_DEPTH_TEST));
}

void Renderer::Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const
{
    shader.Bind();
    va.Bind();
    ib.Bind();
    
    GLCall(glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::DrawLines(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const
{
    shader.Bind();
    va.Bind();
    ib.Bind();

    GLCall(glDrawElements(GL_LINES, ib.GetCount(), GL_UNSIGNED_INT, nullptr));
}

void Renderer::DeleteShaderCache()
{
#ifndef NDEBUG
    std::cout << "Deleting shader cache." << std::endl;
#endif
    m_ShaderCache.erase(m_ShaderCache.begin(), m_ShaderCache.end());
}

void Renderer::DeleteTextureCache()
{
    if (!m_TextureCache.empty())
    {
#ifndef NDEBUG
        std::cout << "Deleting texture cache." << std::endl;
#endif
        m_TextureCache.erase(m_TextureCache.begin(), m_TextureCache.end());
    }
}

void Renderer::Shutdown()
{
    DeleteShaderCache();
    DeleteTextureCache();
}


std::shared_ptr<Shader> Renderer::GetShader(const std::string& filePath)
{
    if (m_ShaderCache.find(filePath) != m_ShaderCache.end())
    {
        return m_ShaderCache[filePath];
    }
    else
    {
        m_ShaderCache[filePath] = std::make_shared<Shader>(filePath);
        return m_ShaderCache[filePath];
    }
}


std::shared_ptr<Texture2D> Renderer::GetTexture(const std::string& filePath, bool flip)
{
    if (m_TextureCache.find(filePath) != m_TextureCache.end())
    {
        return m_TextureCache[filePath];
    }
    else
    {
        m_TextureCache[filePath] = std::make_shared<Texture2D>(filePath, flip);
        return m_TextureCache[filePath];
    }
}
