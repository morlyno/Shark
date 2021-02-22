#include "skpch.h"
#include "OrtographicCameraController.h"

#include "Shark/Core/Input.h"

namespace Shark {

	OrtographicCameraController::OrtographicCameraController(float aspectratio, bool doRotation)
		:
		m_Camera(-aspectratio * m_ZoomLevel, aspectratio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel),
		m_Aspectratio(aspectratio),
		m_DoRotation(doRotation)
	{
	}

	void OrtographicCameraController::OnResize(float width, float height)
	{
		m_Aspectratio = width / height;
		m_Camera.SetProjection(m_Aspectratio * m_ZoomLevel * 2.0f, m_ZoomLevel * 2.0f);
	}

	void OrtographicCameraController::OnUpdate(TimeStep ts)
	{
		if (Input::KeyPressed(Key::W))
			m_Position.y += m_MoveSpeed * ts;

		if (Input::KeyPressed(Key::S))
			m_Position.y -= m_MoveSpeed * ts;

		if (Input::KeyPressed(Key::A))
			m_Position.x -= m_MoveSpeed * ts;

		if (Input::KeyPressed(Key::D))
			m_Position.x += m_MoveSpeed * ts;

		if (m_DoRotation)
		{
			if (Input::KeyPressed(Key::Q))
				m_Rotation -= m_MoveSpeed * ts;

			if (Input::KeyPressed(Key::E))
				m_Rotation += m_MoveSpeed * ts;

			m_Camera.SetRotation(m_Rotation);
		}

		m_Camera.SetPosition(m_Position);
	}

	void OrtographicCameraController::OnEvent(Event& e)
	{
		EventDispacher dispacher(e);
		dispacher.DispachEvent<MouseScrolledEvent>(SK_BIND_EVENT_FN(OrtographicCameraController::OnMouseScrolled));
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(OrtographicCameraController::OnWindowResized));
	}

	bool OrtographicCameraController::OnMouseScrolled(MouseScrolledEvent& event)
	{
		m_ZoomLevel = std::max(m_ZoomLevel + event.GetDelta() * 0.25f, 0.25f);
		m_Camera.SetProjection(-m_Aspectratio * m_ZoomLevel, m_Aspectratio * m_ZoomLevel, -m_ZoomLevel, m_ZoomLevel);
		m_MoveSpeed = m_ZoomLevel;
		return false;
	}

	bool OrtographicCameraController::OnWindowResized(WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0)
			return false;

		OnResize(event.GetWidth(), event.GetHeight());
		return false;
	}

#if 0
	void OrtographicCamera::OnImGui()
	{
		ImGui::Begin("Camera Data");
		ImGui::Text("Position: %.2f, %.2f", m_Position.x, m_Position.y);
		ImGui::Text("Rotation: %.3f", m_Rotation);
		ImGui::Text("ViewProjection:");
		const auto r0 = m_ViewProjection.r[0].m128_f32;
		const auto r1 = m_ViewProjection.r[1].m128_f32;
		const auto r2 = m_ViewProjection.r[2].m128_f32;
		const auto r3 = m_ViewProjection.r[3].m128_f32;
		ImGui::Text("%.6f, %.6f, %.6f, %.6f", r0[0], r0[1], r0[2], r0[3]);
		ImGui::Text("%.6f, %.6f, %.6f, %.6f", r1[0], r1[1], r1[2], r1[3]);
		ImGui::Text("%.6f, %.6f, %.6f, %.6f", r2[0], r2[1], r2[2], r2[3]);
		ImGui::Text("%.6f, %.6f, %.6f, %.6f", r3[0], r3[1], r3[2], r3[3]);
		ImGui::End();
	}
#endif

}