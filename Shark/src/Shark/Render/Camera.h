#pragma once

#include <DirectXMath.h>

namespace Shark {

	class Camera
	{
	public:
		Camera() : m_Projection(DirectX::XMMatrixIdentity()) {}
		Camera(const DirectX::XMMATRIX& projection) : m_Projection(projection) {}
		virtual ~Camera() = default;

		virtual const DirectX::XMMATRIX& GetProjection() const { return m_Projection; }
	protected:
		DirectX::XMMATRIX m_Projection;
	};

}