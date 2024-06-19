#pragma once

#include <vector>

class InputHandler
{
public:
	InputHandler();
	~InputHandler();

	void OnKey(int key, int action);
	void OnMouseScroll(double yoffset);
	void OnMouseMovement(double xpos, double ypos);

	int GetKey(int key) { return m_keyPressed[key]; }
	double GetScroll() const { return m_currentMouseScroll; }
	void ResetScroll() { m_currentMouseScroll = 0; }
	double GetMouseDeltaX() const { return m_deltaX; }
	double GetMouseDeltaY() const { return m_deltaY; }
	void SetMouseDeltaXY(double x, double y) { m_deltaX = x; m_deltaY = y; }
	void SetInitialMouseXY(double x, double y);

private:
	std::vector<int> m_keyPressed;
	double m_currentMouseScroll = 0;
	double m_lastX = -1;
	double m_lastY = -1;
	double m_currentX = -1;
	double m_currentY = -1;
	double m_deltaX = 0;
	double m_deltaY = 0;

};