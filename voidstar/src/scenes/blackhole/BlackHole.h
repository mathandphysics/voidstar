#pragma once

#include "Texture.h"
#include "Shapes.h"
#include "Framebuffer.h"

#include "glm/gtc/matrix_transform.hpp"

#include <vector>
#include <iostream>


struct graphicsPreset {
	float bloomThreshold;
	float bloomBackgroundMultiplier;
	float bloomDiskMultiplier;
	float exposure;
	float gamma;
	float brightnessFromRadius;
	float brightnessFromDiskVel;
	float colourshiftPower;
	float colourshiftMultiplier;
	float colourshiftOffset;
};

class BlackHole
{
public:
	BlackHole();
	~BlackHole();

	void OnUpdate();
	void Draw();
	void PostProcess();

	void CreateScreenQuad();
	void LoadTextures();
	void CompileBHShaders();
	void CompilePostShaders();
	void CreateFBOs();

	void SetShaderUniforms();
	void SetScreenShaderUniforms();

	float CalculateDrawDistance();
	float CalculateISCO(float m, float a);

	void OnImGuiRender();
	void ImGuiRenderStats();
	void ImGuiChooseBH();
	void ImGuiBHProperties();
	void ImGuiChooseODESolver();
	void ImGuiSimQuality();
	void ImGuiDebug();
	void ImGuiCinematic();

	void OnResize();
	void SetProjectionMatrix();
	void SetGraphicsPreset(graphicsPreset preset);
private:
	float m_mass = 1.0f;
	float m_radius = 2.0f * m_mass;
	float m_diskInnerRadius = 2.25f * m_radius;
	float m_diskOuterRadius = 9.0f * m_radius;
	float m_a = 0.6f;

	float m_diskRotationSpeed = 0.1f;
	float m_diskRotationAngle = 0.0f;

	bool m_drawLensing = true;
	bool m_transparentDisk = true;
	bool m_drawBasicDisk = false;
	bool m_useDebugDiskTexture = false;
	bool m_useDebugSphereTexture = false;
	bool m_useSphereTexture = false;

	int m_msaa = 1;

	bool m_useBloom = true;
	float m_bloomThreshold = 0.95f;
	bool m_horizontalPass = true;
	bool m_firstIteration = true;
	int m_presetSelector = 0;
	float m_diskAbsorption = 1.0f;
	float m_bloomBackgroundMultiplier = 0.8f;
	float m_bloomDiskMultiplier = 2.5f;
	float m_exposure = 0.4f;
	float m_gamma = 0.7f;
	float m_brightnessFromRadius = 0.0f;
	float m_brightnessFromDiskVel = 3.0f;
	float m_colourshiftPower = 2.0f;
	float m_colourshiftMultiplier = 4400.0f;
	float m_colourshiftOffset = 0.0f;

	bool m_setImGuiPosition = true;

	std::string m_flatSpacetimeShaderPath = "res/shaders/BlackHoleFlatSpacetime.shader";
	std::string m_kerrBlackHoleShaderPath = "res/shaders/KerrBlackHole.shader";
	std::string m_gravitationalLensingShaderPath = "res/shaders/BlackHoleLensing.shader";
	std::string m_flatShaderPath = "res/shaders/BlackHoleFlat.shader";
	std::string m_gaussianBlurShaderPath = "res/shaders/GaussianBlur.shader";
	std::string m_BloomShaderPath = "res/shaders/FinalBloom.shader";
	std::string m_textureToScreenShaderPath = "res/shaders/TextureToScreen.shader";
	int m_shaderSelector = 0;
	std::string m_selectedShaderString = m_kerrBlackHoleShaderPath;

	Mesh m_quad;
	std::shared_ptr<Framebuffer> m_fbo;
	std::shared_ptr<Framebuffer> m_pingFBO;
	std::shared_ptr<Framebuffer> m_pongFBO;

	std::vector<std::string> m_cubeTexturePaths;
	TextureCubeMap m_cubemap;

	int m_maxSteps = 200;
	float m_stepSize = 0.3f;
	float m_drawDistance = 100.0f;
	float m_epsilon = 0.0001f;
	float m_tolerance = 0.01f;
	int m_ODESolverSelector = 2;

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

	float m_FOVy = 60.0f; // in degrees
	glm::mat4 m_proj;
};

