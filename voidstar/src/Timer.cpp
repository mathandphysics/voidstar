#pragma once

#include "Timer.h"

#include "GLFW/glfw3.h"

Timer::Timer()
{
    m_currentTime = (float)glfwGetTime();
}

Timer::~Timer()
{
}

void Timer::OnUpdate()
{
    float previousTime = m_currentTime;
    m_currentTime = (float)glfwGetTime();
    m_deltaTime = m_currentTime - previousTime;
    m_elapsedTime += m_deltaTime;
}