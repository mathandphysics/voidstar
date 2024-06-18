#pragma once

#include "InputHandler.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

class Camera
{
	public:
		Camera();
		~Camera();

		void OnUpdate();
		void OnImGuiRender();
		void CameraGUI();
		void DebugGUI();

		glm::vec3 GetPosition() const { return m_cameraPos; }
		glm::mat4 GetView();
		glm::mat4 GetProj() const { return m_Proj; }
		glm::mat4 GetSkyboxView();
		void Reset();

		void SetCameraPos(glm::vec3 pos);
		void SetProj();
		void TogglePause();
		void ToggleEulerAngles();

		void SetUserMouseSpeed();
		void SetUserMovementSpeed();
		float GetFOV() const { return m_FOV; }

		glm::quat RotationFromEuler(glm::vec3 eulerAngles);
		void TurnOnEuler();
		void TurnOffEuler();
		void UpdateEuler(float upRotAngle, float rightRotAngle);
		void UpdateNonEuler(float upRotAngle, float rightRotAngle);

		InputHandler& GetInputHandler() { return m_InputHandler; }
	private:
		void BarrelRoll(float roll);
		void MouseMovementLook();
		void CameraMove();
		void ChangeMovementSpeed();


	private:
		float m_UserDefinedMovementSpeed = 1.0f;
		float m_UserDefinedMouseSpeed = 1.0f;
		bool m_InvertedY = false;
		bool m_UseEulerAngles = true;
		bool m_Paused = true;

		const float m_defaultMovementSpeed = 4.0f;
		const float m_movementSpeedIncrement = .4f;
		double m_MovementSpeed = m_UserDefinedMovementSpeed * m_defaultMovementSpeed;
		const float m_defaultMouseSpeed = 0.2f;
		float m_MouseSpeed = m_UserDefinedMouseSpeed * m_defaultMouseSpeed;

		InputHandler m_InputHandler;

		const glm::vec3 m_originalLook = glm::vec3(0.0f, 0.0f, 1.0f);
		const glm::vec3 m_originalUp = glm::vec3(0.0f, 1.0f, 0.0f);
		const glm::vec3 m_originalRight = glm::vec3(-1.0f, 0.0f, 0.0f);
		glm::vec3 m_cameraPos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::quat m_cameraRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		glm::vec3 m_Look = m_originalLook * m_cameraRot;
		glm::vec3 m_Up = m_originalUp * m_cameraRot;
		glm::vec3 m_Right = m_originalRight * m_cameraRot;
		glm::vec3 m_EulerAngles = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::mat4 m_View = glm::lookAt(m_cameraPos, m_cameraPos + m_Look, m_Up);
		float m_FOV = 60.0f; // in degrees
		glm::mat4 m_Proj = glm::perspective(glm::radians(m_FOV), 16.0f / 9.0f, 0.1f, 1000.0f);

};