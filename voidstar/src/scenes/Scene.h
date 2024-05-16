#pragma once

#include <vector>
#include <string>
#include <functional>

namespace scene
{

	class Scene
	{
	public:
		Scene() {}
		virtual ~Scene();

		virtual void OnUpdate() {}
		virtual void OnRender() {}
		virtual void OnImGuiRender() {}
	};


	class SceneManager
	{
	public:
		SceneManager();
		~SceneManager();

		void OnUpdate();
		void OnRender();
		void OnImGuiRender();

		template <typename T>
		void RegisterScene(const std::string& name)
		{
			m_SceneList.push_back(std::make_pair(name, []() { return new T(); }));
		}

		void DeleteCurrentScene();
		void SetCurrentScene(std::function<scene::Scene* ()> scene);
		void SetCurrentSceneByName(const std::string& name);

		std::vector<std::pair<std::string, std::function<scene::Scene* ()>>>& GetSceneList() { return m_SceneList; }

	private:
		Scene* m_CurrentScene;
		std::vector<std::pair<std::string, std::function<scene::Scene* ()>>> m_SceneList;
	};
}