#include "Shapes.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


TriColoured::TriColoured()
{
}

TriColoured::TriColoured(const float* vertData, VertexBufferLayout layout,
                            const std::string& shaderPath, const std::string& texturePath, bool flip)
{
    unsigned int indices[] = { 0, 1, 2 };
    Mesh::Initialize(vertData, 3, layout, indices, 3, shaderPath, texturePath, flip);
}

TriColoured::~TriColoured()
{
}



QuadColoured::QuadColoured()
{
}

QuadColoured::~QuadColoured()
{
}

QuadColoured::QuadColoured(const float* vertData, VertexBufferLayout layout,
                            const std::string& shaderPath, const std::string& texturePath, bool flip)
{
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    Mesh::Initialize(vertData, 4, layout, indices, 6, shaderPath, texturePath, flip);
}



Cube::Cube()
{
}

Cube::Cube(const std::string& shaderPath)
// White cube.
{
    for (int i = 0; i < 6; i++)
    {
        m_FaceColours.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    GenerateGeometry(shaderPath);
}

Cube::Cube(glm::vec4 colour, const std::string& shaderPath)
// Use the same colour for all six faces.
{
    for (int i = 0; i < 6; i++)
    {
        m_FaceColours.push_back(colour);
    }
    GenerateGeometry(shaderPath);
}

Cube::Cube(std::vector<glm::vec4> colours, const std::string& shaderPath)
// Colour order for faces: front, back, left, right, top, bottom.
{
    assert(colours.size() == 6);
    m_FaceColours = colours;
    GenerateGeometry(shaderPath);    
}

Cube::Cube(const std::string& shaderPath, const std::string& path, bool flip)
// Use the same texture for all six faces.
{
    for (int i = 0; i < 6; i++)
    {
        m_FaceColours.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }
    SetTexture(path, flip);
    GenerateGeometry(shaderPath);
}

void Cube::GenerateGeometry(const std::string& shaderPath)
{

    glm::vec2 t0 = glm::vec2(0.0f, 0.0f);
    glm::vec2 t1 = glm::vec2(1.0f, 0.0f);
    glm::vec2 t2 = glm::vec2(1.0f, 1.0f);
    glm::vec2 t3 = glm::vec2(0.0f, 1.0f);

    glm::vec3 v0 = glm::vec3(1.0, -1.0, 1.0);
    glm::vec3 v1 = glm::vec3(1.0, 1.0, 1.0);
    glm::vec3 v2 = glm::vec3(-1.0, 1.0, 1.0);
    glm::vec3 v3 = glm::vec3(-1.0, -1.0, 1.0);
    glm::vec3 v4 = glm::vec3(1.0, -1.0, -1.0);
    glm::vec3 v5 = glm::vec3(1.0, 1.0, -1.0);
    glm::vec3 v6 = glm::vec3(-1.0, 1.0, -1.0);
    glm::vec3 v7 = glm::vec3(-1.0, -1.0, -1.0);

    std::vector<Vertex> vertVec =
    {
        // front face
        { v3, m_FaceColours[0], t0, 1.0f },
        { v0, m_FaceColours[0], t1, 1.0f },
        { v1, m_FaceColours[0], t2, 1.0f },
        { v2, m_FaceColours[0], t3, 1.0f },

        // back face
        { v4, m_FaceColours[1], t0, 1.0f },
        { v7, m_FaceColours[1], t1, 1.0f },
        { v6, m_FaceColours[1], t2, 1.0f },
        { v5, m_FaceColours[1], t3, 1.0f },

        // left face
        { v7, m_FaceColours[2], t0, 1.0f },
        { v3, m_FaceColours[2], t1, 1.0f },
        { v2, m_FaceColours[2], t2, 1.0f },
        { v6, m_FaceColours[2], t3, 1.0f },

        // right face
        { v0, m_FaceColours[3], t0, 1.0f },
        { v4, m_FaceColours[3], t1, 1.0f },
        { v5, m_FaceColours[3], t2, 1.0f },
        { v1, m_FaceColours[3], t3, 1.0f },

        // top face
        { v2, m_FaceColours[4], t0, 1.0f },
        { v1, m_FaceColours[4], t1, 1.0f },
        { v5, m_FaceColours[4], t2, 1.0f },
        { v6, m_FaceColours[4], t3, 1.0f },

        // bottom face
        { v7, m_FaceColours[5], t0, 1.0f },
        { v4, m_FaceColours[5], t1, 1.0f },
        { v0, m_FaceColours[5], t2, 1.0f },
        { v3, m_FaceColours[5], t3, 1.0f }

    };

    unsigned int indices[] = {
        // front face
        0, 1, 2,
        2, 3, 0,

        // back face
        4, 5, 6,
        6, 7, 4,

        // left face
        8, 9, 10,
        10, 11, 8,

        // right face
        12, 13, 14,
        14, 15, 12,

        // top face
        16, 17, 18,
        18, 19, 16,

        // bottom face
        20, 21, 22,
        22, 23, 20
    };

    VertexBufferLayout layout;
    layout.Push<float>(3);
    layout.Push<float>(4);
    layout.Push<float>(2);
    layout.Push<float>(1);

    Mesh::Initialize((const float*) vertVec.data(), 24, layout, indices, 36, shaderPath);
}



Sphere::Sphere()
    : m_Verts(), m_Normals(), m_TexCoords(), m_Indices(), m_Colours()
{
}

Sphere::Sphere(float radius, int numLats, int numLongs, const std::string& shaderPath)
    : m_Verts(), m_Normals(), m_TexCoords(), m_Indices(), m_Colours()
{
    CheckInputs(radius, numLats, numLongs);
    GenerateGeometry(shaderPath);
}


Sphere::Sphere(float radius, int numLats, int numLongs, const std::string& shaderPath, const std::string& path, bool flip)
{
    CheckInputs(radius, numLats, numLongs);
    SetTexture(path, flip);
    GenerateGeometry(shaderPath);
}

Sphere::~Sphere()
{
}

void Sphere::CheckInputs(float radius, int numLats, int numLongs)
{
    assert(radius > 0.0f);
    m_Radius = radius;
    m_invRadius = 1 / radius;

    if (numLats < 2)
    {
        m_numLats = 2;
    }
    else
    {
        m_numLats = numLats;
    }

    if (numLongs < 3)
    {
        m_numLongs = 3;
    }
    else
    {
        m_numLongs = numLongs;
    }
}

void Sphere::GenerateGeometry(const std::string& shaderPath)
{
    unsigned int nVerts = 0;
    unsigned int nInds = 0;
    std::vector<Vertex> vertVec;

    float deltaLatitude = glm::pi<float>() / (float)m_numLats;
    float deltaLongitude = 2 * glm::pi<float>() / (float)m_numLongs;
    float latitudeAngle;
    float longitudeAngle;

    float vx, vy, vz;
    float nx, ny, nz;
    float cx, cy, cz;

    for (int i = 0; i <= m_numLats; i++)
    {
        latitudeAngle = glm::pi<float>() / 2 - i * deltaLatitude;

        float xz = m_Radius * glm::cos(latitudeAngle);
        float y = m_Radius * glm::sin(latitudeAngle);

        for (int j = 0; j <= m_numLongs; j++)
        {
            longitudeAngle = j * deltaLongitude;

            vx = xz * glm::sin(longitudeAngle);
            vy = y;
            vz = xz * glm::cos(longitudeAngle);

            m_Verts.push_back(glm::vec3(vx, vy, vz));
            float tex_x = (float)j / (float)m_numLongs;
            float tex_y = 1.0f - (float)i / (float)m_numLats;
            m_TexCoords.push_back(glm::vec2({ tex_x, tex_y }));

            nx = vx * m_invRadius;
            ny = vy * m_invRadius;
            nz = vz * m_invRadius;
            m_Normals.push_back(glm::vec3(nx, ny, nz));
            cx = (nx + 1.0f) / 2.0f;
            cy = (ny + 1.0f) / 2.0f;
            cz = (nz + 1.0f) / 2.0f;
            m_Colours.push_back(glm::vec4(cx, cy, cz, 1.0f));
            vertVec.push_back({ glm::vec3(vx, vy, vz), glm::vec4(cx , cy, cz, 1.0f), glm::vec2(tex_x, tex_y), 1.0f });
            nVerts += 1;
        }
    }

    /*
     *  Indices
     *  k1--k1+1
     *  |  / |
     *  | /  |
     *  k2--k2+1
     */
    unsigned int k1, k2;
    for (int i = 0; i < m_numLats; i++)
    {
        k1 = i * (m_numLongs + 1);
        //k1 = i * m_numLongs;
        k2 = k1 + (m_numLongs + 1);
        // 2 Triangles per latitude block excluding the first and last longitudes blocks
        for (int j = 0; j < m_numLongs; j++, k1++, k2++)
        {
            if (i != 0)
            {
                m_Indices.push_back(k1);
                m_Indices.push_back(k2);
                m_Indices.push_back(k1 + 1);
                nInds += 3;
            }

            if (i != (m_numLats - 1))
            {
                m_Indices.push_back(k1 + 1);
                m_Indices.push_back(k2);
                m_Indices.push_back(k2 + 1);
                nInds += 3;
            }
        }
    }
    VertexBufferLayout layout;
    layout.Push<float>(3);
    layout.Push<float>(4);
    layout.Push<float>(2);
    layout.Push<float>(1);

    Mesh::Initialize((const float*)vertVec.data(), nVerts, layout,
        (const unsigned int*)m_Indices.data(), nInds, shaderPath);
}