#include "InputHandler.h"

#include "GLFW/glfw3.h"

InputHandler::InputHandler()
	: m_keyPressed(GLFW_KEY_LAST + 1)
{
}

InputHandler::~InputHandler()
{
}

void InputHandler::OnKey(int key, int action)
{
	if (action == GLFW_PRESS) {
		m_keyPressed[key] = 1;
	}
	else if (action == GLFW_RELEASE) {
		m_keyPressed[key] = 0;
	}
}

void InputHandler::OnMouseScroll(double yoffset)
{
	m_currentMouseScroll = yoffset;
}

void InputHandler::OnMouseMovement(double xpos, double ypos)
{
	m_deltaX = xpos - m_currentX;
	m_deltaY = ypos - m_currentY;
	m_lastX = m_currentX;
	m_lastY = m_currentY;
	m_currentX = xpos;
	m_currentY = ypos;
}

void InputHandler::SetInitialMouseXY(double x, double y)
{
	m_lastX = x;
	m_lastY = y;
	m_currentX = x;
	m_currentY = y;
	m_deltaX = 0;
	m_deltaY = 0;
}