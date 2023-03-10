#include "BlackHoleScene.h"
#include "Renderer.h"

scene::BlackHoleScene::BlackHoleScene()
{
}

scene::BlackHoleScene::~BlackHoleScene()
{
}

void scene::BlackHoleScene::OnUpdate()
{
    m_blackHole.OnUpdate();
}

void scene::BlackHoleScene::OnRender()
{
    m_blackHole.Draw();
}

void scene::BlackHoleScene::OnImGuiRender()
{
    m_blackHole.OnImGuiRender();
}