#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Camera.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/MouseEvent.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Core/TimeStep.h"

#include <glm/glm.hpp>

namespace Shark {

	class EditorCamera : public Camera
	{
	public:
		EditorCamera();
		EditorCamera(float aspectratio, float fov, float nearClip, float farClip);
		virtual ~EditorCamera() = default;

		void SetProjection(float aspectratio, float fov, float nearClip, float farClip);
		glm::mat4 GetViewProjection() const { return m_Projection * m_View; }
		const glm::mat4& GetView() const { return m_View; }

		void Resize(float width, float height) { m_ViewportSize = { width, height }; m_AspectRatio = width / height; UpdateProjection(); }
		
		const glm::vec2& GetViewportSize() const { return m_ViewportSize; }
		float GetViewporWidth() const { return m_ViewportSize.x; }
		float GetViewportHeight() const { return m_ViewportSize.y; }

		void SetAspectRatio(float aspectratio) { m_AspectRatio = aspectratio; UpdateProjection(); }
		void SetFOV(float fov) { m_FOV = glm::radians(fov); UpdateProjection(); }
		void SetFarClip(float farClip) { m_FarClip = farClip; UpdateProjection(); }
		void SetNearClip(float nearClip) { m_NearClip = nearClip; UpdateProjection(); }

		float GetAspectRatio() const { return m_AspectRatio; }
		float GetFOV() const { return glm::degrees(m_FOV); }
		float GetFarClip() const { return m_FarClip; }
		float GetNearClip() const { return m_NearClip; }


		void SetFlyView(const glm::vec3& position, float pitch, float yaw);
		void SetView(const glm::vec3& focuspoint, float distance, float pitch, float yaw) { m_FocusPoint = focuspoint; m_Distance = distance; m_Pitch = pitch; m_Yaw = yaw; UpdatePosition(); UpdateView(); }
		void SetFocusPoint(const glm::vec3& focuspoint) { m_FocusPoint = focuspoint; UpdatePosition(); UpdateView(); }
		void SetDistance(float distance) { m_Distance = distance; UpdatePosition(); UpdateView(); }
		void SetPicht(float pitch) { m_Pitch = glm::radians(pitch); UpdatePosition(); UpdateView(); }
		void SetYaw(float yaw) { m_Yaw = glm::radians(yaw); UpdatePosition(); UpdateView(); }
		
		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::vec3& GetFocusPoint() const { return m_FocusPoint; }
		float GetDistance() const { return m_Distance; }
		float GetPitch() const { return glm::degrees(m_Pitch); }
		float GetYaw() const { return glm::degrees(m_Yaw); }

		float GetFlySpeed() const { return m_MoveSpeed; }
		void SetFlySpeed(float speed) { m_MoveSpeed = speed; }

		float GetSpeedup() const { return m_SpeedupFactor; }
		void SetSpeedup(float speedup) { m_SpeedupFactor = speedup; }

		float GetSlowdown() const { return m_SlowdownFactor; }
		void SetSlowdown(float slowdown) { m_SlowdownFactor = slowdown; }

		float GetRotationSpeed() const { return m_RotateSpeed; }
		void SetRotationSpeed(float rotationSpeed) { m_RotateSpeed = rotationSpeed; }

		bool GetFlyMode() const { return m_FlyMode; }

		void OnUpdate(TimeStep ts, bool update);
		void OnEvent(Event& event);

	private:
		bool OnMouseScolledEvent(MouseScrolledEvent& event);
		bool OnKeyPressedEvent(KeyPressedEvent& event);

	private:
		glm::vec3 GetForwardDirection() const;
		glm::vec3 GetUpwardsDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetLeftDirection() const;
		glm::vec3 GetBackwardsDirection() const;
		glm::vec3 GetDownwardsDirection() const;
		glm::quat GetRotation() const;

		glm::vec2 GetMoveSpeed();
		float GetZoomSpeed();

		void UpdateView();
		void UpdateProjection();
		void UpdatePosition();
		void UpdateFocusPoint();

		void OnMouseRotate(const glm::vec2& delta);
		void OnMouseMove(const glm::vec2& delta);
		void OnMouseZoom(const glm::vec2& delta);

		// Dosn't update the View Matrix. Zo apply the changes call EditorCamera::UpdateView.
		void Move(const glm::vec3& direction, float delta);
		void Rotate(const glm::vec2& delta);
	private:
		bool m_Update = false;
		bool m_FlyMode = false;

		float m_MoveSpeed = 10.0f;
		float m_RotateSpeed = 0.5f;
		float m_SpeedupFactor = 6.0f;
		float m_SlowdownFactor = 0.5f;

		float m_AspectRatio = 16.0f / 9.0f;
		float m_FOV = glm::radians(45.0f);
		float m_NearClip = 0.1f;
		float m_FarClip = 1000.0f;

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		glm::mat4 m_View;
		glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 m_FocusPoint = { 0.0f, 0.0f, 0.0f };

		glm::vec2 m_LastMousePos = { 0.0f, 0.0f };

		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
	};

}