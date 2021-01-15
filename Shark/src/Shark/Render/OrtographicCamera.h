#pragma once

#include "Shark/Core/Base.h"
#include <DirectXMath.h>

namespace Shark {

	class OrtographicCamera
	{
	public:
		OrtographicCamera(float left, float right, float bottem, float top);
		void SetProjection(float left, float right, float bottem, float top);
		void SetProjection(float width, float height);
		const DirectX::XMMATRIX& GetViewProjection() const { return m_ViewProjection; }
		
		const DirectX::XMFLOAT3& GetPosition() const { return m_Position; }
		void SetPosition(const DirectX::XMFLOAT3 position) { m_Position = position; RecalculateViewProjection(); }
		
		void SetRotation(float rotation) { m_Rotation = rotation; RecalculateViewProjection(); }
		float GetRotation() const { return m_Rotation; }
	private:
		void RecalculateViewProjection();

	private:
		DirectX::XMMATRIX m_Projection;
		DirectX::XMMATRIX m_View;
		DirectX::XMMATRIX m_ViewProjection;

		DirectX::XMFLOAT3 m_Position;
		float m_Rotation = 0.0f;
	};

}