#include "skpch.h"
#include "OrtographicCamera.h"

namespace Shark {

	OrtographicCamera::OrtographicCamera(float left, float right, float bottem, float top)
		:
		m_Position(0.0f, 0.0f, 0.0f),
		m_Projection(DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottem, top, -1.0f, 1.0f)),
		m_View(DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z))
	{
		RecalculateViewProjection();
	}

	void OrtographicCamera::SetProjection(float left, float right, float bottem, float top)
	{
		m_Projection = DirectX::XMMatrixOrthographicOffCenterLH(left, right, bottem, top, -1.0f, 1.0f);

		RecalculateViewProjection();
	}

	void OrtographicCamera::SetProjection(float width, float height)
	{
		m_Projection = DirectX::XMMatrixOrthographicLH(width, height, -1.0f, 1.0f);
	}

	void OrtographicCamera::RecalculateViewProjection()
	{
		m_View = DirectX::XMMatrixRotationZ(m_Rotation) *
			     DirectX::XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);

		DirectX::XMMATRIX inverseView = DirectX::XMMatrixInverse(nullptr, m_View);

		m_ViewProjection = inverseView * m_Projection;
	}

}
