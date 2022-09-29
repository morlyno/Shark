#include "skpch.h"
#include "EditorCamera.h"

#include "Shark/Input/Input.h"
#include "Shark/Debug/Instrumentor.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Shark {

	EditorCamera::EditorCamera()
	{
		UpdatePosition();
		UpdateProjection();
		UpdateView();
	}

	EditorCamera::EditorCamera(float aspectratio, float fov, float nearClip, float farClip)
		: Camera(glm::perspective(glm::radians(fov), aspectratio, nearClip, farClip)),
		m_AspectRatio(aspectratio), m_FOV(glm::radians(fov)), m_NearClip(nearClip), m_FarClip(farClip)
	{
		UpdatePosition();
		UpdateView();
	}

	void EditorCamera::SetProjection(float aspectratio, float fov, float nearClip, float farClip)
	{
		m_AspectRatio = aspectratio;
		m_FOV = glm::radians(fov);
		m_NearClip = nearClip;
		m_FarClip = farClip;

		UpdateProjection();
	}

	void EditorCamera::OnUpdate(TimeStep ts)
	{
		if (Input::IsKeyDown(KeyCode::LeftAlt))
		{
			glm::vec2 mousePos = Input::GetMousePosition();
			glm::vec2 delta = (mousePos - m_LastMousePos) * 0.003f;
			m_LastMousePos = mousePos;
			if (Input::IsMouseDown(MouseButton::Left))
				OnMouseRotate(delta);
			else if (Input::IsMouseDown(MouseButton::Right))
				OnMouseZoom(delta);
			else if (Input::IsMouseDown(MouseButton::Middle))
				OnMouseMove(delta);
		}
	}

	void EditorCamera::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<MouseScrolledEvent>(SK_BIND_EVENT_FN(EditorCamera::OnMouseScolledEvent));
	}

	bool EditorCamera::OnMouseScolledEvent(MouseScrolledEvent& event)
	{
		static constexpr float scroll = 7.5f;
		m_Distance -= event.GetDelta() * 7.5f;
		if (m_Distance < 0.25)
			m_Distance = 0.25;

		UpdatePosition();
		UpdateView();

		return true;
	}

	glm::vec3 EditorCamera::GetForwardDirection() const
	{
		return glm::rotate(GetRotation(), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	glm::vec3 EditorCamera::GetUpwardsDirection() const
	{
		return glm::rotate(GetRotation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCamera::GetRightDirection() const
	{
		return glm::rotate(GetRotation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::quat EditorCamera::GetRotation() const
	{
		return glm::quat(glm::vec3(m_Pitch, m_Yaw, 0.0f));
	}

	glm::vec2 EditorCamera::GetMoveSpeed()
	{
		float x = std::min(m_ViewportSize.x / 1000.0f, 2.4f); // max = 2.4f
		//float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportSize.y / 1000.0f, 2.4f); // max = 2.4f
		//float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::GetZoomSpeed()
	{
		float distance = m_Distance * 0.35f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		//speed = std::min(speed, 100.0f); // max speed = 100
		speed = std::clamp(speed, 2.0f, 100.0f);
		return speed;
	}

	void EditorCamera::UpdateView()
	{
		m_View = glm::lookAt(m_Position, m_FocusPoint, glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void EditorCamera::UpdateProjection()
	{
		m_Projection = glm::perspective(m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
	}

	void EditorCamera::UpdatePosition()
	{
		m_Position = m_FocusPoint - GetForwardDirection() * m_Distance;
	}

	void EditorCamera::OnMouseRotate(const glm::vec2& delta)
	{
		m_Pitch += delta.y;
		m_Yaw += delta.x;

		UpdatePosition();
		UpdateView();
	}

	void EditorCamera::OnMouseMove(const glm::vec2& delta)
	{
		glm::vec2 speed = GetMoveSpeed();

		auto r = -GetRightDirection() * delta.x * speed.x * m_Distance;
		auto u = GetUpwardsDirection() * delta.y * speed.y * m_Distance;
		m_Position += r;
		m_Position += u;
		m_FocusPoint += r;
		m_FocusPoint += u;

		UpdateView();
	}

	void EditorCamera::OnMouseZoom(const glm::vec2& delta)
	{
		m_Distance += delta.y * GetZoomSpeed();
		if (m_Distance < 0.25f)
			m_Distance = 0.25f;
		UpdatePosition();
		UpdateView();
	}

}