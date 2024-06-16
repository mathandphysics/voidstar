#include "Camera.h"

#include "Application.h"

#include <imgui.h>

#define CAMERA_DEBUG 0
#if CAMERA_DEBUG
	#include <iostream>
#endif


Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::OnUpdate()
{
	if (!m_Paused)
	{
		ChangeMovementSpeed();
		MouseMovementLook();
		CameraMove();
		m_View = glm::lookAt(m_cameraPos, m_cameraPos + m_Look, m_Up);
	}

#if CAMERA_DEBUG
	std::cout << "Camera position = " << m_cameraPos.x << ", " << m_cameraPos.y << ", " << m_cameraPos.z << "\n";
	std::cout << "Look direction = " << m_Look.x << ", " << m_Look.y << ", " << m_Look.z << "\n";
#endif

}

void Camera::OnImGuiRender()
{
		ImGuiWindowFlags imgui_window_flags = ImGuiWindowFlags_NoCollapse;// | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
		imgui_window_flags |= ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;
		ImGui::Begin("Camera Controls", NULL, imgui_window_flags);
		CameraGUI();
		ImGui::End();
}

void Camera::CameraGUI()
{
	if (ImGui::SliderFloat("##Mouse Speed", &m_UserDefinedMouseSpeed, 0.1f, 1.0f, "Mouse Speed = %.2f"))
	{
		SetUserMouseSpeed();
	}
	if (ImGui::SliderFloat("##Movement Speed", &m_UserDefinedMovementSpeed, 0.1f, 1.0f, "Movement Speed = %.2f"))
	{
		SetUserMovementSpeed();
	}
	if (ImGui::SliderFloat("##FOV", &m_FOV, 20.0f, 120.0f, "FOV = %.1f"))
	{
		SetProj();
	}
	if (ImGui::Checkbox("Invert Camera Y-Axis", &m_InvertedY))
	{
	}
	if (ImGui::Checkbox("Use Euler Angles", &m_UseEulerAngles))
	{
		ToggleEulerAngles();
	}
}

void Camera::DebugGUI()
{
	ImGui::Text("Position.x = %.3f", (float)m_cameraPos.x);
	ImGui::Text("Position.y = %.3f", (float)m_cameraPos.y);
	ImGui::Text("Position.z = %.3f", (float)m_cameraPos.z);
	ImGui::Text("Look.x = %.3f", (float)m_Look.x);
	ImGui::Text("Look.y = %.3f", (float)m_Look.y);
	ImGui::Text("Look.z = %.3f", (float)m_Look.z);
}

glm::mat4 Camera::GetView()
{
	return glm::lookAt(m_cameraPos, m_cameraPos + m_Look, m_Up);
}

glm::mat4 Camera::GetSkyboxView()
{
	return glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), m_Look, m_Up);
}

void Camera::Reset()
{
	m_MovementSpeed = m_UserDefinedMovementSpeed * m_defaultMovementSpeed;
	m_MouseSpeed = m_UserDefinedMouseSpeed * m_defaultMouseSpeed;
	m_cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
	m_cameraRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	m_Look = m_cameraRot * m_originalLook;
	m_Up = m_cameraRot * m_originalUp;
	m_Right = m_cameraRot * m_originalRight;
	m_EulerAngles = glm::vec3(0.0f, 0.0f, 0.0f);
	m_View = glm::lookAt(m_cameraPos, m_cameraPos + m_Look, m_Up);
	m_InputHandler.ResetScroll();
}

void Camera::SetCameraPos(glm::vec3 pos)
{
	m_cameraPos = pos;
}

void Camera::SetProj()
{
	int width, height;
	glfwGetWindowSize(Application::Get().GetWindow().GetWindow(), &width, &height);
	m_Proj = glm::perspective(glm::radians(m_FOV), (float)width / (float)height, 0.1f, 1000.0f);
}

void Camera::TogglePause()
{
	m_Paused = !m_Paused;
}

void Camera::ToggleEulerAngles()
{
	if (m_UseEulerAngles)
	{
		TurnOnEuler();
	}
	else
	{
		TurnOffEuler();
	}
}

void Camera::SetUserMouseSpeed()
{
	m_MouseSpeed = m_UserDefinedMouseSpeed * m_defaultMouseSpeed;
}

void Camera::SetUserMovementSpeed()
{
	m_MovementSpeed = m_UserDefinedMovementSpeed * m_defaultMovementSpeed;
}

void Camera::TurnOnEuler()
{
	m_Up = m_originalUp;
	m_Look = glm::normalize(m_Look); // Make sure m_Look is normalized before we find theta and phi.
	float phi = glm::asin(m_Look.y);
	float theta = glm::atan(m_Look.x / m_Look.z);
	if (m_Look.z < 0)
	{
		// Because arctan only depends on the relative signs of m_Look.x and m_Look.z, we need to correct theta.
		theta += glm::pi<float>();
	}
	m_EulerAngles = glm::vec3(phi, theta, 0.0f);
	m_Look = glm::vec3(glm::sin(m_EulerAngles.y) * glm::cos(m_EulerAngles.x),
		glm::sin(m_EulerAngles.x),
		glm::cos(m_EulerAngles.y) * glm::cos(m_EulerAngles.x));
	m_Up = m_originalUp;
	m_Right = glm::normalize(glm::cross(m_Look, m_Up));
}

void Camera::TurnOffEuler()
{
	if (m_Look == m_originalLook)
	{
		m_Look = m_originalLook;
		m_Up = m_originalUp;
		m_Right = m_originalRight;
	}
	else
	{
		m_Look = glm::normalize(m_Look);
		glm::vec3 axis = glm::normalize(glm::cross(m_originalLook, m_Look));
		float angle = glm::acos(glm::dot(m_originalLook, m_Look));
		m_cameraRot = glm::angleAxis(angle, axis);
		m_Look = m_cameraRot * m_originalLook;
		m_Up = m_cameraRot * m_originalUp;
		m_Right = m_cameraRot * m_originalRight;
	}
}

void Camera::UpdateEuler(float upRotAngle, float rightRotAngle)
{
	constexpr float max_angle = (89.0f / 90.0f) * (glm::pi<float>() / 2.0f);  // 89 degrees
	// Restrict the pitch axis in order to prevent gimbal lock.
	if ((m_EulerAngles.x - rightRotAngle) < max_angle && (m_EulerAngles.x - rightRotAngle) > -max_angle)
	{
		m_EulerAngles.x += -rightRotAngle;
	}
	else
	{
		m_EulerAngles.x = max_angle * glm::sign(m_EulerAngles.x);
	}
	m_EulerAngles.y += -upRotAngle;
	m_Look = glm::vec3(glm::sin(m_EulerAngles.y) * glm::cos(m_EulerAngles.x),
		glm::sin(m_EulerAngles.x),
		glm::cos(m_EulerAngles.y) * glm::cos(m_EulerAngles.x));
	m_Up = m_originalUp;
	m_Right = glm::normalize(glm::cross(m_Look, m_Up));
}

void Camera::UpdateNonEuler(float upRotAngle, float rightRotAngle)
{
	m_cameraRot = glm::normalize(m_cameraRot);
	m_cameraRot = glm::angleAxis(-upRotAngle, m_Up) * m_cameraRot;
	m_cameraRot = glm::angleAxis(-rightRotAngle, m_Right) * m_cameraRot;
	m_Up = glm::normalize(m_cameraRot * m_originalUp);
	m_Look = glm::normalize(m_cameraRot * m_originalLook);
	m_Right = glm::normalize(m_cameraRot * m_originalRight);
}

void Camera::MouseMovementLook()
{
	float deltaTime = Application::Get().GetTimer().GetDeltaTime();
	double deltaX = m_InputHandler.GetMouseDeltaX();
	double deltaY = m_InputHandler.GetMouseDeltaY();
	m_InputHandler.SetMouseDeltaXY(0,0);

	float upRotAngle = (float)deltaX * deltaTime * m_MouseSpeed;
	float rightRotAngle = (1 - 2 * (int)m_InvertedY) * (float)deltaY * deltaTime * m_MouseSpeed;

	if (m_UseEulerAngles)
	{
		UpdateEuler(upRotAngle, rightRotAngle);
	}
	else {
		UpdateNonEuler(upRotAngle, rightRotAngle);
	}
}

void Camera::CameraMove()
{
	float deltaTime = Application::Get().GetTimer().GetDeltaTime();
	glm::vec3 forward;
	if (m_UseEulerAngles)
	{
		forward = glm::normalize(glm::vec3(m_Look.x, 0, m_Look.z));
	}
	else
	{
		forward = m_Look;
	}

	if (m_InputHandler.GetKey(GLFW_KEY_W) == 1)
	{
		m_cameraPos += (float)m_MovementSpeed * deltaTime * forward;
	}

	if (m_InputHandler.GetKey(GLFW_KEY_S) == 1)
	{
		m_cameraPos -= (float)m_MovementSpeed * deltaTime * forward;
	}

	if (m_InputHandler.GetKey(GLFW_KEY_D) == 1)
	{
		m_cameraPos += (float)m_MovementSpeed * deltaTime * m_Right;
	}

	if (m_InputHandler.GetKey(GLFW_KEY_A) == 1)
	{
		m_cameraPos -= (float)m_MovementSpeed * deltaTime * m_Right;
	}

	if (m_InputHandler.GetKey(GLFW_KEY_SPACE) == 1)
	{
		m_cameraPos += (float)m_MovementSpeed * deltaTime * m_Up;
	}

	if (m_InputHandler.GetKey(GLFW_KEY_LEFT_SHIFT) == 1)
	{
		m_cameraPos -= (float)m_MovementSpeed * deltaTime * m_Up;
	}

}

void Camera::ChangeMovementSpeed()
{
	double scroll = m_InputHandler.GetScroll();
	if (scroll != 0)
	{
		m_MovementSpeed += m_movementSpeedIncrement * scroll;
		if (m_MovementSpeed < 0)
		{
			m_MovementSpeed = 0;
		}
		m_InputHandler.ResetScroll();
	}
}