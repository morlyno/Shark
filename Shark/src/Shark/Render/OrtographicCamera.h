#pragma once

#include "Shark/Core/Core.h"
#include "Shark/Core/TimeStep.h"
#include <DirectXMath.h>

namespace Shark {

	class OrtographicCamera
	{
	public:
		OrtographicCamera(float left, float right, float bottem, float top);

		void OnUpdate(TimeStep ts);
		void OnImGui();

		void SetProjection(float left, float right, float bottem, float top);
		void SetPosition(const DirectX::XMFLOAT3 position) { m_Position = position; RecalculateViewProjection(); }

		const DirectX::XMMATRIX& GetViewProjection() const { return m_ViewProjection; }

	private:
		void RecalculateViewProjection();

	private:
		DirectX::XMMATRIX m_Projection;
		DirectX::XMMATRIX m_View;
		DirectX::XMMATRIX m_ViewProjection;

		DirectX::XMFLOAT3 m_Position;
		const float m_MoveSpeed = 1.0f;

		float m_Rotation = 0.0f;
		const float m_RotationSpeed = 1.0f;

	};

}