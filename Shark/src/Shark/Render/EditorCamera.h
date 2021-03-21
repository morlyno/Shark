#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Camera.h"
#include "Shark/Event/Event.h"
#include "Shark/Core/TimeStep.h"

#include <DirectXMath.h>

namespace Shark {

	class EditorCamera : public Camera
	{
	public:
		EditorCamera();
		EditorCamera(float aspectratio, float fov, float nearClip, float farClip);
		virtual ~EditorCamera() = default;

		void SetProjection(float aspectratio, float fov, float nearClip, float farClip);
		DirectX::XMMATRIX GetViewProjection() const { return m_View * m_Projection; }


		void Resize(float width, float height) { m_ViewportSize = { width, height }; m_AspectRatio = width / height; UpdateProjection(); }
		
		const DirectX::XMFLOAT2& GetViewportSize() const { return m_ViewportSize; }
		float GetViewporWidth() const { return m_ViewportSize.x; }
		float GetViewportHeight() const { return m_ViewportSize.y; }

		
		void SetAspectRatio(float aspectratio) { m_AspectRatio = aspectratio; UpdateProjection(); }
		void SetFOV(float fov) { m_FOV = DirectX::XMConvertToRadians(fov); UpdateProjection(); }
		void SetFarClip(float farClip) { m_FarClip = farClip; UpdateProjection(); }
		void SetNearClip(float nearClip) { m_NearClip = nearClip; UpdateProjection(); }

		float GetAspectRatio() const { return m_AspectRatio; }
		float GetFOV() const { return DirectX::XMConvertToDegrees(m_FOV); }
		float GetFarClip() const { return m_FarClip; }
		float GetNearClip() const { return m_NearClip; }


		void SetFocusPoint(const DirectX::XMFLOAT3& focuspoint) { m_FocusPoint = focuspoint; UpdatePosition(); UpdateView(); }
		void SetDistance(float distance) { m_Distance = distance; UpdatePosition(); UpdateView(); }
		void SetPicht(float pitch) { m_Pitch = DirectX::XMConvertToRadians(pitch); UpdatePosition(); UpdateView(); }
		void SetYaw(float yaw) { m_Yaw = DirectX::XMConvertToRadians(yaw); UpdatePosition(); UpdateView(); }
		
		const DirectX::XMFLOAT3& GetPosition() const { return m_Position; }
		const DirectX::XMFLOAT3& GetFocusPoint() const { return m_FocusPoint; }
		float GetDistance() const { return m_Distance; }
		float GetPitch() const { return DirectX::XMConvertToDegrees(m_Pitch); }
		float GetYaw() const { return DirectX::XMConvertToDegrees(m_Yaw); }


		void OnUpdate(TimeStep ts);
		void OnEvent(Event& event);
	private:
		DirectX::XMVECTOR GetForwardDirection() const;
		DirectX::XMVECTOR GetUpwardsDirection() const;
		DirectX::XMVECTOR GetRightDirection() const;
		DirectX::XMVECTOR GetRotation() const;

		DirectX::XMFLOAT2 GetMoveSpeed();
		float GetZoomSpeed();

		void UpdateView();
		void UpdateProjection();
		void UpdatePosition();

		void OnMouseRotate(const DirectX::XMFLOAT2& delta);
		void OnMouseMove(const DirectX::XMFLOAT2& delta);
		void OnMouseZoom(const DirectX::XMFLOAT2& delta);
	private:
		float m_AspectRatio = 1.77778f, m_FOV = DirectX::XMConvertToRadians(45), m_NearClip = 0.01f, m_FarClip = 1000.0f;
		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;

		DirectX::XMMATRIX m_View;
		DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 m_FocusPoint = { 0.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT2 m_LastMousePos = { 0.0f, 0.0f };

		DirectX::XMFLOAT2 m_ViewportSize = { 0.0f, 0.0f };
	};

}