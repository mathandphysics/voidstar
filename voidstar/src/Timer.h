#pragma once

class Timer
{
public:
	Timer();
	~Timer();

	void OnUpdate();
	void Start();
	void Stop();
	float GetCurrentTime() { return m_currentTime; }
	float GetDeltaTime() { return m_deltaTime; }
	float GetAverageDeltaTime() { return m_averageDeltaTime; }
	float GetElapsedTime() { return m_elapsedTime; }

private:
	float m_beta = 0.95f;
	float m_previousTime = 0.0f;
	float m_currentTime = 0.0f;
	float m_deltaTime = 0.0f;
	float m_averageDeltaTime = 0.0f;
	float m_elapsedTime = 0.0f;
};