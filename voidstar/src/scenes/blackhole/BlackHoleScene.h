#pragma once

#include "scenes/Scene.h"

#include "BlackHole.h"

namespace scene
{

	class BlackHoleScene : public Scene
	{
	public:
		BlackHoleScene();
		~BlackHoleScene();

		void OnUpdate() override;
		void OnRender() override;
		void OnImGuiRender() override;

	private:
		BlackHole m_blackHole;
	};

}