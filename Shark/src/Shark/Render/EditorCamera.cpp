#include "skpch.h"
#include "EditorCamera.h"

#include "Shark/Core/Input.h"
#include "Shark/Event/MouseEvent.h"

namespace Shark {

	EditorCamera::EditorCamera()
	{
		UpdateProjection();
		UpdateView();
	}

	EditorCamera::EditorCamera(float aspectratio, float fov, float nearClip, float farClip)
		: Camera(DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(fov), aspectratio, nearClip, farClip)),
		m_AspectRatio(aspectratio), m_FOV(DirectX::XMConvertToRadians(fov)), m_NearClip(nearClip), m_FarClip(farClip)
	{
		UpdateView();
	}

	void EditorCamera::SetProjection(float aspectratio, float fov, float nearClip, float farClip)
	{
		m_AspectRatio = aspectratio;
		m_FOV = DirectX::XMConvertToRadians(fov);
		m_NearClip = nearClip;
		m_FarClip = farClip;

		UpdateProjection();
	}

	void EditorCamera::OnUpdate(TimeStep ts)
	{
		if (Input::KeyPressed(Key::Alt))
		{
			float x = (float)Input::MousePosX();
			float y = (float)Input::MousePosY();
			DirectX::XMFLOAT2 delta = { (x - m_LastMousePos.x) * 0.003f, (y - m_LastMousePos.y) * 0.003f };
			m_LastMousePos = { x, y };
			if (Input::MousePressed(Mouse::LeftButton))
				OnMouseRotate(delta);
			else if (Input::MousePressed(Mouse::RightButton))
				OnMouseZoom(delta);
			else if (Input::MousePressed(Mouse::Middle))
				OnMouseMove(delta);
		}
	}

	void EditorCamera::OnEvent(Event& event)
	{
		if (event.GetEventType() == MouseScrolledEvent::GetStaticType())
		{
			auto& mse = static_cast<MouseScrolledEvent&>(event);
			m_Distance += mse.GetDelta() * 0.25f;
			if (m_Distance < 0.25f)
				m_Distance = 0.25f;

			UpdatePosition();
			UpdateView();
		}
	}

	DirectX::XMVECTOR EditorCamera::GetForwardDirection() const
	{
		return DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&m_FocusPoint), DirectX::XMLoadFloat3(&m_Position)));
	}

	DirectX::XMVECTOR EditorCamera::GetUpwardsDirection() const
	{
		return DirectX::XMVector3Normalize(DirectX::XMVector3Cross(GetForwardDirection(), GetRightDirection()));
	}

	DirectX::XMVECTOR EditorCamera::GetRightDirection() const
	{
		return DirectX::XMVector3Normalize(DirectX::XMVector3Rotate(GetForwardDirection(), DirectX::XMQuaternionRotationAxis(DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), DirectX::g_XMHalfPi[0])));
	}

	DirectX::XMVECTOR EditorCamera::GetRotation() const
	{
		return DirectX::XMQuaternionRotationRollPitchYaw(m_Pitch, m_Yaw, 0.0f);
	}

	DirectX::XMFLOAT2 EditorCamera::GetMoveSpeed()
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
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::UpdateView()
	{
		m_View = DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&m_Position), DirectX::XMLoadFloat3(&m_FocusPoint), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	}

	void EditorCamera::UpdateProjection()
	{
		m_Projection = DirectX::XMMatrixPerspectiveFovLH(m_FOV, m_AspectRatio, m_NearClip, m_FarClip);
	}

	void EditorCamera::UpdatePosition()
	{
		auto pos = DirectX::XMVector3Transform(DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMMatrixTranslation(0.0f, 0.0f, -m_Distance) *
			DirectX::XMMatrixRotationQuaternion(GetRotation()) * DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&m_FocusPoint)));
		DirectX::XMStoreFloat3(&m_Position, pos);
	}

	void EditorCamera::OnMouseRotate(const DirectX::XMFLOAT2& delta)
	{
		m_Pitch += delta.y;
		m_Yaw += delta.x;

		UpdatePosition();
		UpdateView();
	}

	void EditorCamera::OnMouseMove(const DirectX::XMFLOAT2& delta)
	{
		using namespace DirectX;
		auto pos = DirectX::XMLoadFloat3(&m_Position);
		auto focuspos = DirectX::XMLoadFloat3(&m_FocusPoint);

		auto [xSpeed, ySpeed] = GetMoveSpeed();

		auto r = -GetRightDirection() * delta.x * xSpeed * m_Distance;
		auto u = GetUpwardsDirection() * delta.y * ySpeed * m_Distance;
		pos += r;
		pos += u;
		focuspos += r;
		focuspos += u;

		XMStoreFloat3(&m_Position, pos);
		XMStoreFloat3(&m_FocusPoint, focuspos);
		
		UpdateView();
	}

	void EditorCamera::OnMouseZoom(const DirectX::XMFLOAT2& delta)
	{
		m_Distance += delta.y * GetZoomSpeed();
		if (m_Distance < 0.25f)
			m_Distance = 0.25f;
		UpdatePosition();
		UpdateView();
	}

}