#include "skpch.h"
#include "OrtographicCamera.h"

#include "Shark/Core/Input.h"

#include <imgui.h>

namespace Shark {

	OrtographicCamera::OrtographicCamera(float left, float right, float bottem, float top)
		:
		m_Position(0.0f, 0.0f, 0.0f),
		m_Projection(DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottem, top, -1.0f, 1.0f)),
		m_View(DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z))
	{
		RecalculateViewProjection();
	}

	void OrtographicCamera::OnUpdate(TimeStep ts)
	{
		if (Input::KeyPressed(Key::Up))
			m_Position.y += m_MoveSpeed * ts;
		
		if (Input::KeyPressed(Key::Down))
			m_Position.y -= m_MoveSpeed * ts;

		if (Input::KeyPressed(Key::Left))
			m_Position.x -= m_MoveSpeed * ts;
		
		if (Input::KeyPressed(Key::Right))
			m_Position.x += m_MoveSpeed * ts;

		if (Input::KeyPressed(Key::A))
			m_Rotation -= m_MoveSpeed * ts;
		
		if (Input::KeyPressed(Key::D))
			m_Rotation += m_MoveSpeed * ts;

		RecalculateViewProjection();
	}

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

	void OrtographicCamera::SetProjection(float left, float right, float bottem, float top)
	{
		m_Projection = DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottem, top, -1.0f, 1.0f);

		RecalculateViewProjection();
	}

	void OrtographicCamera::RecalculateViewProjection()
	{
		m_View = DirectX::XMMatrixRotationZ(m_Rotation) *
			     DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

		DirectX::XMMATRIX inverseView = DirectX::XMMatrixInverse(nullptr, m_View);

		m_ViewProjection = inverseView * m_Projection;
	}

}
