#include "Camera.h"

#include "Application.h"

#include <imgui.h>


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
}

void Camera::OnImGuiRender()
{
		ImGuiWindowFlags imgui_window_flags = ImGuiWindowFlags_NoCollapse;
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
	if (ImGui::SliderFloat("##FOV", &m_FOV, 20.0f, 150.0f, "FOV = %.1f"))
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

#ifndef NDEBUG
void Camera::DebugGUI()
{
	ImGui::Text("Position.x = %.3f", (float)m_cameraPos.x);
	ImGui::Text("Position.y = %.3f", (float)m_cameraPos.y);
	ImGui::Text("Position.z = %.3f", (float)m_cameraPos.z);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Look.x = %.3f", (float)m_Look.x);
	ImGui::Text("Look.y = %.3f", (float)m_Look.y);
	ImGui::Text("Look.z = %.3f", (float)m_Look.z);
	ImGui::Separator();
	ImGui::Text("Right.x = %.3f", (float)m_Right.x);
	ImGui::Text("Right.y = %.3f", (float)m_Right.y);
	ImGui::Text("Right.z = %.3f", (float)m_Right.z);
	ImGui::Separator();
	ImGui::Text("Up.x = %.3f", (float)m_Up.x);
	ImGui::Text("Up.y = %.3f", (float)m_Up.y);
	ImGui::Text("Up.z = %.3f", (float)m_Up.z);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("Euler Angle X = %.3f", m_EulerAngles.x);
	ImGui::Text("Euler Angle Y = %.3f", m_EulerAngles.y);
	ImGui::Text("Euler Angle Z = %.3f", m_EulerAngles.z);
	ImGui::Separator();
	ImGui::Separator();
	ImGui::Text("m_cameraRot.x = %.3f", m_cameraRot.x);
	ImGui::Text("m_cameraRot.y = %.3f", m_cameraRot.y);
	ImGui::Text("m_cameraRot.z = %.3f", m_cameraRot.z);
	ImGui::Text("m_cameraRot.w = %.3f", m_cameraRot.w);
	ImGui::Separator();
	ImGui::Separator();
	if (ImGui::Checkbox("Use Euler Angles", &m_UseEulerAngles))
	{
		ToggleEulerAngles();
	}
}
#endif

glm::mat4 Camera::GetView() const
{
	return glm::lookAt(m_cameraPos, m_cameraPos + m_Look, m_Up);
}

glm::mat4 Camera::GetSkyboxView() const
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

glm::quat Camera::RotationFromEuler(glm::vec3 eulerAngles)
{
	// Euler angles are all calculated relative to the original Look, Up, and Right.  Multiply the return value of this function
	// on the left of a vector, e.g. retval * vec3, so that the order of application is ZRot * YRot * XRot * vec3,
	// i.e. so that the X rotation (pitch) is applied to the vector first, then the Y rotation (yaw), and finally
	// the Z rotation (roll).

	// Note that if we wanted to multiply on the right, we'd need to flip the sign of the euler angle, e.g. like
	// vec3 * glm::angleAxis(-eulerAngles.x, m_originalRight);
	glm::quat XRot = glm::angleAxis(eulerAngles.x, m_originalRight);
	glm::quat YRot = glm::angleAxis(eulerAngles.y, m_originalUp);
	glm::quat ZRot = glm::angleAxis(eulerAngles.z, m_originalLook);
	return glm::normalize(ZRot * YRot * XRot);
}

void Camera::TurnOnEuler()
{
	m_Up = m_originalUp;
	// Make sure m_Look is normalized before we find pitch and yaw.
	m_Look = glm::normalize(m_Look);
	float pitch = glm::asin(m_Look.y);
	float yaw = glm::atan(m_Look.x, m_Look.z);

	// Restrict the pitch axis in order to prevent gimbal lock.
	constexpr float max_angle = (89.0f / 90.0f) * (glm::pi<float>() / 2.0f);  // 89 degrees in radians
	if (pitch > max_angle || pitch < -max_angle)
	{
		pitch = max_angle * glm::sign(pitch);
	}

	// We can't use a roll angle because we are forcing up to be fixed.
	m_EulerAngles = glm::vec3(pitch, yaw, 0.0f);
	// This should keep m_Look unchanged if the Euler angles were calculated properly.
	m_Look = RotationFromEuler(m_EulerAngles) * m_originalLook;
	m_Up = m_originalUp;
	m_Right = glm::normalize(glm::cross(m_Look, m_Up));
}

void Camera::TurnOffEuler()
{
	// Convert to a quaternion-based local coordinate system without a fixed up direction.
	m_cameraRot = RotationFromEuler(m_EulerAngles);
	m_Look = m_cameraRot * m_originalLook;
	m_Up = m_cameraRot * m_originalUp;
	m_Right = m_cameraRot * m_originalRight;
}

void Camera::UpdateEuler(float yaw, float pitch)
{
	constexpr float max_angle = (89.0f / 90.0f) * (glm::pi<float>() / 2.0f);  // 89 degrees in radians

	// Restrict the pitch axis in order to prevent gimbal lock.
	if ((m_EulerAngles.x + pitch) < max_angle && (m_EulerAngles.x + pitch) > -max_angle)
	{
		m_EulerAngles.x += pitch;
	}
	else
	{
		m_EulerAngles.x = max_angle * glm::sign(m_EulerAngles.x);
	}
	m_EulerAngles.y += yaw;

	m_Look = RotationFromEuler(m_EulerAngles) * m_originalLook;
	m_Up = m_originalUp;
	m_Right = glm::normalize(glm::cross(m_Look, m_Up));
}

void Camera::UpdateNonEuler(float yaw, float pitch)
{
	// Apply pitch to the existing rotation first, then yaw.
	m_cameraRot = glm::angleAxis(yaw, glm::cross(m_Right, m_Look)) * glm::angleAxis(pitch, m_Right) * m_cameraRot;
	m_cameraRot = glm::normalize(m_cameraRot);
	m_Up = m_cameraRot * m_originalUp;
	m_Look = m_cameraRot * m_originalLook;
	m_Right = m_cameraRot * m_originalRight;
}

void Camera::BarrelRoll(float roll)
{
	if (!m_UseEulerAngles)
	{
		m_cameraRot = glm::angleAxis(roll, m_Look) * m_cameraRot;
	}
}

void Camera::MouseMovementLook()
{
	float deltaTime = Application::Get().GetTimer().GetDeltaTime();
	double deltaX = m_InputHandler.GetMouseDeltaX();
	double deltaY = m_InputHandler.GetMouseDeltaY();
	m_InputHandler.SetMouseDeltaXY(0,0);

	float yaw = -(float)deltaX * deltaTime * m_MouseSpeed;
	float pitch = (1.0f - 2.0f * (float)m_InvertedY) * -(float)deltaY * deltaTime * m_MouseSpeed;

	if (m_UseEulerAngles)
	{
		UpdateEuler(yaw, pitch);
	}
	else {
		UpdateNonEuler(yaw, pitch);
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

	if (m_InputHandler.GetKey(GLFW_KEY_Q) == 1)
	{
		BarrelRoll(-deltaTime);
	}
	if (m_InputHandler.GetKey(GLFW_KEY_E) == 1)
	{
		BarrelRoll(deltaTime);
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
	float scroll = (float)m_InputHandler.GetScroll();
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