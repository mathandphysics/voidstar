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
    m_previousTime = m_currentTime;
    m_currentTime = (float)glfwGetTime();
    m_deltaTime = m_currentTime - m_previousTime;
    m_averageDeltaTime = m_averageDeltaTime * m_beta + m_deltaTime * (1 - m_beta);
    m_elapsedTime += m_deltaTime;
}

void Timer::Start()
{
    m_currentTime = (float)glfwGetTime();
}

void Timer::Stop()
{
    m_deltaTime = (float)glfwGetTime() - m_currentTime;
    m_averageDeltaTime = m_averageDeltaTime * m_beta + m_deltaTime * (1 - m_beta);
}
