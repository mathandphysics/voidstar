#pragma once

#include "Texture.h"
#include "Shapes.h"
#include "Framebuffer.h"

#include "glm/gtc/matrix_transform.hpp"

#include <vector>
#include <iostream>

class BlackHole
{
public:
	BlackHole();
	~BlackHole();

	void OnUpdate();
	void Draw();
	void SetShaderUniforms();
	void SetScreenShaderUniforms();
	void PostProcess();

	void OnImGuiRender();
private:
	float m_mass = 1.0f;
	float m_radius = 2.0f * m_mass;
	float m_diskInnerRadius = 3.0f * m_radius;
	float m_diskOuterRadius = 9.0f * m_radius;
	float m_a = 0.6f;

	bool m_rotateDisk = true;
	float m_diskRotationSpeed = 0.1f;
	float m_diskRotationAngle = 0.0f;

	bool m_drawLensing = true;
	bool m_useDebugDiskTexture = false;
	bool m_useDebugSphereTexture = false;
	bool m_useBloom = false;
	bool m_horizontalPass = true;
	bool m_firstIteration = true;
	float m_exposure = 1.0f;
	Framebuffer m_pingFBO;
	Framebuffer m_pongFBO;

	std::string m_kerrBlackHoleShaderPath = "res/shaders/KerrBlackHole.shader";
	std::string m_gravitationalLensingShaderPath = "res/shaders/BlackHoleLensing.shader";
	std::string m_flatShaderPath = "res/shaders/BlackHoleFlat.shader";
	std::string m_gaussianBlurShaderPath = "res/shaders/GaussianBlur.shader";
	std::string m_BloomShaderPath = "res/shaders/FinalBloom.shader";
	std::string m_textureToScreenShaderPath = "res/shaders/TextureToScreen.shader";

	QuadColoured m_quad;
	Framebuffer m_fbo;

	std::vector<std::string> m_cubeTexturePaths;
	TextureCubeMap m_cubemap;

	int m_maxSteps = 1000;
	float m_stepSize = 0.1f;
	float m_maxDistance = 100.0f;
	float m_epsilon = 0.0001f;

	glm::vec3 m_diskDebugColourTop1 = glm::vec3(0.1, 0.8, 0.2);
	glm::vec3 m_diskDebugColourTop2 = glm::vec3(0.02, 0.47, 0.87);
	glm::vec3 m_diskDebugColourBottom1 = glm::vec3(0.98, 0.4, 0.0);
	glm::vec3 m_diskDebugColourBottom2 = glm::vec3(1.0, 0.84, 0.0);
	std::string m_diskTexturePath;
	std::shared_ptr<Texture2D> m_diskTexture;

	glm::vec3 m_sphereDebugColour1 = glm::vec3(0.8, 0.0, 0.0);
	glm::vec3 m_sphereDebugColour2 = glm::vec3(0.30, 0.05, 0.46);
	std::string m_sphereTexturePath;
	std::shared_ptr<Texture2D> m_sphereTexture;

	unsigned int m_diskTextureSlot = 4;
	unsigned int m_sphereTextureSlot = 2;
	unsigned int m_skyboxTextureSlot = 3;
	unsigned int m_screenTextureSlot = 0;

	glm::mat4 m_proj = glm::perspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
};