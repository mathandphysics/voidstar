#include "Scene.h"

#include "imgui.h"

#include "blackhole/BlackHoleScene.h"

#include "Application.h"


scene::Scene::~Scene()
{
	Renderer::Get().DeleteTextureCache();  // Let the shader cache persist.
	Application::Get().GetCamera().Reset();
}




scene::SceneManager::SceneManager()
{
	scene::SceneManager::RegisterScene<scene::BlackHoleScene>("Black Hole");
}

scene::SceneManager::~SceneManager()
{
	scene::SceneManager::DeleteCurrentScene();
}

void scene::SceneManager::OnUpdate()
{
	if (m_CurrentScene)
	{
		m_CurrentScene->OnUpdate();
	}
}

void scene::SceneManager::OnRender()
{
	Renderer::Get().ClearWithDepth();
	if (m_CurrentScene)
	{
		m_CurrentScene->OnRender();
	}
}

void scene::SceneManager::OnImGuiRender()
{
	if (m_CurrentScene)
	{
		m_CurrentScene->OnImGuiRender();
	}
}

void scene::SceneManager::DeleteCurrentScene()
{
	if (m_CurrentScene)
	{
#ifdef _DEBUG
		std::cout << "Deleting current scene." << std::endl;
#endif
		delete m_CurrentScene;
	}
}

void scene::SceneManager::SetCurrentScene(std::function<scene::Scene* ()> scene)
{
	m_CurrentScene = scene();
}

void scene::SceneManager::SetCurrentSceneByName(const std::string& name)
{
	for (auto& scene : m_SceneList)
	{
		if (name == scene.first)
		{
			scene::SceneManager::SetCurrentScene(scene.second);
		}
	}
}
