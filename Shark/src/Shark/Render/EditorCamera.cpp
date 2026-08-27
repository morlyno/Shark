#include "skpch.h"
#include "EditorCamera.h"

#include "Shark/Input/Input.h"
#include "Shark/Event/Event.h"
#include "Shark/Event/KeyEvent.h"
#include "Shark/Event/MouseEvent.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/optimum_pow.hpp>

namespace Shark {

	EditorCamera::EditorCamera(float degFov, float width, float height, float nearClip, float farClip)
		: m_VerticalFOV(glm::radians(degFov)), m_ViewportWidth(width), m_ViewportHeight(height), m_NearClip(nearClip), m_FarClip(farClip)
	{
		SetPerspectiveProjectionMatrix(m_VerticalFOV, m_ViewportWidth / m_ViewportHeight, m_NearClip, m_FarClip);

		m_Position = { -10.0f, 5.0f, -10.0f };
		Focus(glm::zero<glm::vec3>());

		m_PitchDelta = 0.0f;
		m_YawDelta = 0.0f;
		m_PositionDelta = glm::zero<glm::vec3>();

		m_Position = m_FocusPoint - GetForwardDirection() * m_Distance;
		m_ViewMatrix = glm::lookAt(m_Position, m_FocusPoint, { 0.0f, 1.0f, 0.0f });
	}

	void EditorCamera::OnUpdate(TimeStep ts)
	{
		glm::vec2 mouse = Input::GetMousePosition();
		glm::vec2 mouseDelta = mouse - m_LastMousePosition;
		
		m_LastMousePosition = mouse;

		if (!m_Active && !m_Animated)
		{
			if (m_CameraMode != CameraMode::None)
			{
				Input::SetCursorMode(CursorMode::Normal);
				m_CameraMode = CameraMode::None;
			}

			return;
		}

		CameraMode currentMode = CameraMode::None;

		if (m_Active)
		{
			if (m_CameraMode == CameraMode::Fly)
			{
				currentMode = CameraMode::Fly;
				const float speed = CameraSpeed();

				auto movement = glm::zero<glm::vec3>();
				if (Input::IsKeyDown(KeyCode::W)) movement += speed * ts * GetForwardDirection();
				if (Input::IsKeyDown(KeyCode::S)) movement -= speed * ts * GetForwardDirection();
				if (Input::IsKeyDown(KeyCode::A)) movement -= speed * ts * GetRightDirection();
				if (Input::IsKeyDown(KeyCode::D)) movement += speed * ts * GetRightDirection();
				if (Input::IsKeyDown(KeyCode::E)) movement += speed * ts * glm::vec3(0.0f, 1.0f, 0.0f);
				if (Input::IsKeyDown(KeyCode::Q)) movement += speed * ts * glm::vec3(0.0f, -1.0f, 0.0f);

				m_Position += movement;
				m_PositionDelta += movement;
				m_Animated = true;

				auto yaw = mouseDelta.x * m_MouseSensitivity * ts;
				auto pitch = mouseDelta.y * m_MouseSensitivity * ts;

				m_Yaw += yaw;
				m_Pitch += pitch;
				m_FocusPoint = m_Position + GetForwardDirection() * m_Distance;
			}
			else if (m_CameraMode == CameraMode::Arcball)
			{
				currentMode = CameraMode::Arcball;
				auto delta = mouseDelta * 0.003f;

				if (Input::IsMouseDown(MouseButton::Left))
				{
					Input::SetCursorMode(CursorMode::Locked);
					auto yaw = delta.x * 0.5f;
					auto pitch = delta.y * 0.5f;

					m_Yaw += yaw;
					m_Pitch += pitch;
					m_Position = m_FocusPoint - GetForwardDirection() * m_Distance;
				}
				else if (Input::IsMouseDown(MouseButton::Right))
				{
					Input::SetCursorMode(CursorMode::Locked);
					MouseZoom(delta.y * ZoomSpeed(), true);
				}
				else if (Input::IsMouseDown(MouseButton::Middle))
				{
					Input::SetCursorMode(CursorMode::Locked);
					auto worldHeight = 2.0f * m_Distance * glm::tan(m_VerticalFOV * 0.5f);
					auto worldWidth = worldHeight * (m_ViewportWidth / m_ViewportHeight);

					float x = worldWidth / m_ViewportWidth;
					float y = worldHeight / m_ViewportHeight;

					auto movement = -GetRightDirection() * mouseDelta.x * x +
						GetUpwardsDirection() * mouseDelta.y * y;

					m_FocusPoint += movement;
					m_Position += movement;
					m_PositionDelta += movement;
					m_Animated = true;
				}
			}
		}

		if (currentMode != CameraMode::None || m_Animated)
		{
			const auto position = m_Position - m_PositionDelta;
			const auto focus    = position + GetSmoothForwardDirection() * m_Distance;

			m_ViewMatrix = glm::lookAt(position, focus, glm::vec3(0.0f, 1.0f, 0.0f));

			if (glm::epsilonEqual(m_YawDelta, 0.0f, 0.000001f) &&
				glm::epsilonEqual(m_PitchDelta, 0.0f, 0.000001f) &&
				glm::all(glm::epsilonEqual(m_PositionDelta, glm::vec3(0.0f), glm::vec3(0.000001f))))
			{
				m_Animated = false;
				m_YawDelta = 0.0f;
				m_PitchDelta = 0.0f;
				m_PositionDelta = glm::vec3(0.0f);
			}
			else
			{
				m_YawDelta *= 0.8f;
				m_PitchDelta *= 0.8f;
				m_PositionDelta *= 0.8;
			}
		}
	}

	void EditorCamera::OnEvent(Event& event)
	{
		if (!m_Active)
			return;

		EventDispacher dispacher(event);
		dispacher.DispachEvent<MouseScrolledEvent>([this](MouseScrolledEvent& event)
		{
			MouseZoom(-event.GetYOffset() * 5.0f, true);
			return true;
		});

		dispacher.DispachEvent<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event)
		{
			if (event.GetButton() == MouseButton::Right && m_CameraMode == CameraMode::None)
			{
				m_CameraMode = CameraMode::Fly;
				Input::SetCursorMode(CursorMode::Locked);
				return true;
			}
			return false;
		});

		dispacher.DispachEvent<KeyPressedEvent>([this](KeyPressedEvent& event)
		{
			if (event.IsRepeat())
				return false;

			const auto keyCode = event.GetKeyCode();
			if (keyCode == KeyCode::Escape && m_CameraMode == CameraMode::Fly)
			{
				m_CameraMode = CameraMode::None;
				Input::SetCursorMode(CursorMode::Normal);
				return true;
			}

			if (keyCode == KeyCode::LeftAlt && m_CameraMode == CameraMode::None)
			{
				m_CameraMode = CameraMode::Arcball;
				return true;
			}

			return false;
		});

		dispacher.DispachEvent<KeyReleasedEvent>([this](KeyReleasedEvent& event)
		{
			if (event.GetKeyCode() == KeyCode::LeftAlt && m_CameraMode == CameraMode::Arcball)
			{
				m_CameraMode = CameraMode::None;
				Input::SetCursorMode(CursorMode::Normal);
				return true;
			}
			return false;
		});
	}

	void EditorCamera::Resize(float width, float height)
	{
		if (m_ViewportWidth == width && m_ViewportHeight == height)
			return;

		m_ViewportWidth = width;
		m_ViewportHeight = height;
		SetPerspectiveProjectionMatrix(m_VerticalFOV, width, height, m_NearClip, m_FarClip);
	}

	void EditorCamera::Focus(const glm::vec3& focusPoint)
	{
		auto direction = glm::normalize(focusPoint - m_Position);
		auto distance = glm::distance(focusPoint, m_Position);

		static constexpr float MIN_FOCUS_DISTANCE = 10.0f;
		static constexpr float MAX_FOCUS_DISTANCE = 50.0f;

		auto position = focusPoint - direction * m_Distance;
		auto pitch = glm::asin(-direction.y);
		auto yaw = glm::atan(direction.x, direction.z);


		m_PositionDelta = position - m_Position;
		m_PitchDelta = pitch - m_Pitch;
		m_YawDelta = glm::mod(yaw - m_Yaw + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
		m_Animated = true;

		m_FocusPoint = focusPoint;
		m_Position = position;
		m_Pitch = pitch;
		m_Yaw = yaw;
	}

	void EditorCamera::SetPosition(const glm::vec3& position, const glm::vec3& forwardDirection, std::optional<float> focusDistance, bool animate)
	{
		auto direction = glm::normalize(forwardDirection);
		auto distance = focusDistance.value_or(m_Distance);
		auto focusPoint = position + direction * distance;
		auto pitch = glm::asin(-direction.y);
		auto yaw = glm::atan(direction.x, direction.z);

		if (animate)
		{
			m_PositionDelta = position - m_Position;
			m_PitchDelta = pitch - m_Pitch;
			m_YawDelta = glm::mod(yaw - m_Yaw + glm::pi<float>(), glm::two_pi<float>()) - glm::pi<float>();
		}

		// always set m_Animated true to update view matrix
		// this will update for one frame as all deltas are 0
		m_Animated = true;

		m_Distance = glm::distance(focusPoint, position);
		m_FocusPoint = focusPoint;
		m_Position = position;
		m_Pitch = pitch;
		m_Yaw = yaw;
	}

	float EditorCamera::ZoomSpeed()
	{
		return std::clamp(glm::pow2(m_Distance * 0.35f),
						  2.0f,
						  100.0f);
	}

	float EditorCamera::CameraSpeed()
	{
		float speed = m_NormalSpeed;
		if (Input::IsKeyDown(KeyCode::LeftShift))
			speed *= m_SpeedupFactor;
		if (Input::IsKeyDown(KeyCode::LeftAlt))
			speed *= m_SlowdownFactor;

		return speed;
	}

	void EditorCamera::MouseZoom(float delta, bool move)
	{
		m_Distance += delta;

		const auto forwardDirection = GetForwardDirection();
		auto position = m_FocusPoint - forwardDirection * m_Distance;

		if (m_Distance < 1.0f)
		{
			m_Distance = 1.0f;
			if (move)
			{
				m_FocusPoint += forwardDirection * m_Distance;
			}
		}


		m_Animated = true;
		m_PositionDelta += position - m_Position;
		m_Position = position;
	}

}