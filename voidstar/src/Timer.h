#pragma once

class Timer
{
public:
	Timer();
	~Timer();

	void OnUpdate();
	float GetCurrentTime() { return m_currentTime; }
	float GetDeltaTime() { return m_deltaTime; }
	float GetElapsedTime() { return m_elapsedTime; }

private:
	float m_currentTime = 0.0f;
	float m_deltaTime = 0.0f;
	float m_elapsedTime = 0.0f;
};