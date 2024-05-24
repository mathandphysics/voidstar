#pragma once

#include <glad/glad.h>
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"
#include "Texture.h"

#include <memory>

#define ASSERT(x) if (!(x)) __debugbreak();
#ifndef NDEBUG
#define GLCall(x) GLClearError(); x; ASSERT(GLLogCall(#x, __FILE__, __LINE__))
#else
#define GLCall(x) x
#endif

void GLClearError();

bool GLLogCall(const char* function, const char* file, int line);

void APIENTRY GLDebugOutput(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam);

void EnableOpenGLDebugging();


class Renderer
{
public:
    Renderer(const Renderer&) = delete;
    ~Renderer();

    void Clear() const;
    void ClearWithDepth() const;
    void EnableDepth() const;
    void DisableDepth() const;

    void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const;
    void DrawLines(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const;

    void DeleteShaderCache();
    void DeleteTextureCache();

    void Shutdown();

    static Renderer& Get()
    {
        static Renderer s_Renderer;
        return s_Renderer;
    }

    std::shared_ptr<Shader> GetShader(const std::string& filePath);
    std::shared_ptr<Texture2D> GetTexture(const std::string& filePath, bool flip);

private:
    Renderer();
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_ShaderCache;
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> m_TextureCache;
};