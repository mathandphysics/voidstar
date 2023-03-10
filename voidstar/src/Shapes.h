#pragma once

#include "Mesh.h"

#include <glm/glm.hpp>
#include <memory>
#include <vector>


class TriColoured : public Mesh
{
public:
	TriColoured();
	TriColoured(const float* vertData, VertexBufferLayout layout,
		const std::string& shaderPath, const std::string& texturePath = "", bool flip = true);
	~TriColoured();

private:

};


class QuadColoured : public Mesh
{
public:
	QuadColoured();
	QuadColoured(const float* vertData, VertexBufferLayout layout,
		const std::string& shaderPath, const std::string& texturePath = "", bool flip = true);
	~QuadColoured();

private:

};


class Cube : public Mesh
{
public:
	Cube();
	Cube(const std::string& shaderPath);
	Cube(glm::vec4 colour, const std::string& shaderPath);
	Cube(std::vector<glm::vec4> colour, const std::string& shaderPath);
	Cube(const std::string& shaderPath, const std::string& texturePath, bool flip);

private:
	void GenerateGeometry(const std::string& shaderPath);

	std::vector<glm::vec3> m_Verts;
	std::vector<unsigned int> m_Indices;
	std::vector<glm::vec4> m_FaceColours;
};


class Sphere : public Mesh
{
public:
	Sphere();
	Sphere(float radius, int numLats, int numLongs, const std::string& shaderPath);
	Sphere(float radius, int numLats, int numLongs, const std::string& shaderPath, const std::string& path, bool flip);
	~Sphere();

	void CheckInputs(float radius, int numLats, int numLongs);

private:
	void GenerateGeometry(const std::string& shaderPath);

	float m_Radius = 0.0f;
	float m_invRadius = 0.0f;
	int m_numLats = 0;
	int m_numLongs = 0;
	std::vector<glm::vec3> m_Verts;
	std::vector<glm::vec3> m_Normals;
	std::vector<glm::vec2> m_TexCoords;
	std::vector<unsigned int> m_Indices;
	std::vector<glm::vec4> m_Colours;
};