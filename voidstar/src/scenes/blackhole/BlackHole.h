#pragma once

#include "Texture.h"
#include "Shapes.h"
#include "Framebuffer.h"

#include "glm/gtc/matrix_transform.hpp"

#include <vector>
#include <iostream>


struct graphicsPreset {
	std::string name;
	float innerRadius;
	float a;
	float temp;
	float diskAbsorption;
	float backgroundBrightness;
	float diskBrightness;
	float dMdt;
	float blueshiftPOW;
	float diskDopplerBrightness;
	bool useBloom;
	float bloomThreshold;
	float exposure;
	float gamma;
};

class BlackHole
{
public:
	BlackHole();
	~BlackHole();

	void OnUpdate();
	void OnClick(int x, int y);
	void Draw();
	void PostProcess();

	void CreateScreenQuad();
	void LoadTextures();
	void CompileBHShaders();
	void CompilePostShaders() const;
	void CreateFBOs();

	void SetShaderUniforms();
	void SetScreenShaderUniforms();

	float CalculateKerrDistance(const glm::vec3 p) const;
	float CalculateDrawDistance();
	void CalculateISCO();

	void OnImGuiRender();
	void ImGuiSetUpDocking();
	void ImGuiRenderStats();
	void ImGuiChooseBH();
	void ImGuiBHProperties();
	void ImGuiChooseODESolver();
	void ImGuiSimQuality();
	void ImGuiDebug();
	void ImGuiCinematic();

	void OnResize();
	void SetProjectionMatrix();
	void SetGraphicsPreset(const graphicsPreset &preset);
	void SetShader(const std::string& filePath);
	void SetShaderDefines();

private:
	float m_mass = 1.0f;
	float m_dMdt = 10000.0; // dM/dt = \dot{M}
	float m_risco;
	float m_diskInnerRadius = 4.5f * m_mass;
	float m_diskOuterRadius = 18.0f * m_mass;
	float m_a = 0.6f;
	float m_Tmax = 2000.0f;
	bool m_insideHorizon = false;

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
	float m_diskAbsorption = 1.0f;
	float m_bloomBackgroundMultiplier = 0.8f;
	float m_bloomDiskMultiplier = 2.5f;
	float m_exposure = 0.4f;
	float m_gamma = 0.7f;
	float m_blueshiftPower = 1.0;
	float m_brightnessFromDiskVel = 4.0f;

	int m_presetSelector = 0;
	std::vector<graphicsPreset> m_presets = { { "Warm", 4.5f, 0.6f, 2000.0f, 7.0f, 0.0f, 1.0f, 15000.0f, 1.0f, 6.0f, false, 1.6f, 0.5f, 0.5f },
		{ "Interstellar", 4.5f, 0.6f, 4500.0f, 7.0f, 0.0f, 1.0f, 15000.0f, 0.0f, 1.0f, true, 0.1f, 1.0f, 0.5f },
		{ "Realistic", 3.85f, 0.6f, 10000.0f, 3.0f, 0.0f, 1.0f, 5000.0f, 1.0f, 4.0f, true, 4.0f, 1.0f, 0.5f },
	};

	bool m_ImGuiFirstTime = true;
	bool m_ImGuiAllowMoveableDock = true;

	std::string m_kerrBlackHoleShaderPath = "res/shaders/KerrBlackHole.shader";
	std::string m_gaussianBlurShaderPath = "res/shaders/GaussianBlur.shader";
	std::string m_BloomShaderPath = "res/shaders/FinalBloom.shader";
	int m_shaderSelector = 0;
	std::string m_selectedShaderString = m_kerrBlackHoleShaderPath;
	std::vector<std::string> m_vertexDefines = {};
	std::vector<std::string> m_fragmentDefines = {};

	Mesh m_quad;
	std::shared_ptr<Framebuffer> m_fbo;
	std::shared_ptr<Framebuffer> m_pingFBO;
	std::shared_ptr<Framebuffer> m_pongFBO;

	std::vector<std::string> m_cubeTexturePaths = {
		// Ordering of faces must be: xpos, xneg, ypos, yneg, zpos, zneg.
		"res/textures/px.png",
		"res/textures/nx.png",
		"res/textures/py.png",
		"res/textures/ny.png",
		"res/textures/pz.png",
		"res/textures/nz.png"
	};
	TextureCubeMap m_cubemap;
	std::string m_diskTexturePath;
	std::shared_ptr<Texture2D> m_diskTexture;
	std::string m_sphereTexturePath;
	std::shared_ptr<Texture2D> m_sphereTexture;

	int m_maxSteps = 200;
	float m_drawDistance = 100.0f;
	float m_tolerance = 0.01f;
	float m_diskIntersectionThreshold = 0.001f;
	float m_sphereIntersectionThreshold = 0.001f;
	int m_ODESolverSelector = 2;

	glm::vec3 m_diskDebugColourTop1 = glm::vec3(0.1, 0.8, 0.2);
	glm::vec3 m_diskDebugColourTop2 = glm::vec3(0.02, 0.47, 0.87);
	glm::vec3 m_diskDebugColourBottom1 = glm::vec3(0.98, 0.4, 0.0);
	glm::vec3 m_diskDebugColourBottom2 = glm::vec3(1.0, 0.84, 0.0);

	glm::vec3 m_sphereDebugColour1 = glm::vec3(0.8, 0.0, 0.0);
	glm::vec3 m_sphereDebugColour2 = glm::vec3(0.30, 0.05, 0.46);


	unsigned int m_diskTextureSlot = 4;
	unsigned int m_sphereTextureSlot = 2;
	unsigned int m_skyboxTextureSlot = 3;
	unsigned int m_screenTextureSlot = 0;
};

