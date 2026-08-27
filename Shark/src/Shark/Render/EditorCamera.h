#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Render/Camera.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Shark {
	class Event;
}

namespace Shark {

	enum class CameraMode
	{
		None = 0,
		Fly,
		Arcball
	};

	class EditorCamera : public Camera
	{
	public:
		EditorCamera(float degFov, float width, float height, float nearClip, float farClip);
		virtual ~EditorCamera() = default;

		bool IsActive() const { return m_Active; }
		void SetActive(bool active) { m_Active = active; }
		CameraMode GetCurrentMode() const { return m_CameraMode; }

		void OnUpdate(TimeStep ts);
		void OnEvent(Event& event);
		void Resize(float width, float height);

		void Focus(const glm::vec3& focusPoint);
		void SetPosition(const glm::vec3& position, const glm::vec3& forwardDirection, std::optional<float> focusDistance = std::nullopt, bool animate = true);
		//void SetDistance(float distance);

		float GetDistance() const { return m_Distance; }
		const glm::vec3& GetPosition() const { return m_Position; }
		const glm::vec3& GetFocusPoint() const { return m_FocusPoint; }

		const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
		glm::mat4 GetViewProjection() const { return GetProjection() * m_ViewMatrix; }

	private:
		glm::vec3 GetSmoothForwardDirection() const { return GetSmoothDirection() * glm::vec3(0.0f, 0.0f, 1.0f); }
		glm::quat GetSmoothDirection()        const { return glm::quat(glm::vec3(m_Pitch - m_PitchDelta, m_Yaw - m_YawDelta, 0.0f)); }

		glm::vec3 GetUpwardsDirection() const { return GetDirection() * glm::vec3(0.0f, 1.0f, 0.0f); }
		glm::vec3 GetForwardDirection() const { return GetDirection() * glm::vec3(0.0f, 0.0f, 1.0f); }
		glm::vec3 GetRightDirection()   const { return GetDirection() * glm::vec3(1.0f, 0.0f, 0.0f); }
		glm::quat GetDirection()        const { return glm::quat(glm::vec3(m_Pitch, m_Yaw, 0.0f)); }

		float ZoomSpeed();
		float CameraSpeed();

		void MouseZoom(float delta, bool move);
	private:
		glm::mat4 m_ViewMatrix;
		glm::vec3 m_Position, m_FocusPoint;
		glm::vec2 m_LastMousePosition = { 0.0f, 0.0f };

		float m_VerticalFOV;
		float m_NearClip, m_FarClip;
		float m_ViewportWidth, m_ViewportHeight;

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		float m_MouseSensitivity = 0.2f;
		float m_NormalSpeed = 50.0f;
		float m_SlowdownFactor = 0.2f;
		float m_SpeedupFactor = 3.0f;

		glm::vec3 m_PositionDelta = glm::vec3(0.0f);
		float m_YawDelta = 0.0f;
		float m_PitchDelta = 0.0f;
		bool m_Animated = false;

		bool m_Active = false;
		CameraMode m_CameraMode = CameraMode::None;

		friend class EditorLayer;
	};

}