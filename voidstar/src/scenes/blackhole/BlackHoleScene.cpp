#include "BlackHoleScene.h"

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

void scene::BlackHoleScene::OnResize()
{
    m_blackHole.OnResize();
}

void scene::BlackHoleScene::OnClick(int x, int y)
{
    m_blackHole.OnClick(x, y);
}
