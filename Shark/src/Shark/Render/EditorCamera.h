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
		void SetDistance(float distance) { m_Distance = distance; UpdatePosition(); UpdateView(); }

		float GetAspectRatio() { return m_AspectRatio; }
		float GetFOV() { return DirectX::XMConvertToDegrees(m_FOV); }
		float GetFarClip() { return m_FarClip; }
		float GetNearClip() { return m_NearClip; }
		float GetDistance() { return m_Distance; }

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
		DirectX::XMFLOAT3 m_Position = { 0.0f, 0.0f, -m_Distance };
		DirectX::XMFLOAT3 m_FocusPoint = { 0.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT2 m_LastMousePos = { 0.0f, 0.0f };

		DirectX::XMFLOAT2 m_ViewportSize = { 0.0f, 0.0f };
	};

}