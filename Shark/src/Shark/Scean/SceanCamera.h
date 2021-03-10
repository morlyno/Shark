#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Camera.h"

#include <DirectXMath.h>

namespace Shark {

	class SceanCamera : public Camera
	{
	public:
		enum class Projection { Perspective, Orthographic };
	public:
		SceanCamera();
		SceanCamera(const Camera& camera);
		SceanCamera(const DirectX::XMMATRIX& projection);

		void SetProjectionType(Projection projection) { m_ProjectionType = projection; Recalcualte(); }
		Projection GetProjectionType() const { return m_ProjectionType; }

		void SetPerspective(float aspectratio, float fov, float clipnear, float clipfar) { m_Aspectratio = aspectratio; m_PerspectiveFOV = DirectX::XMConvertToRadians(fov), m_PerspectiveNear = clipnear, m_PerspectiveFar = clipfar; Recalcualte(); }
		void SetOrthographic(float aspectratio, float zoom, float clipnear, float clipfar) { m_Aspectratio = aspectratio; m_OrthographicZoom = zoom; m_OrthographicNear = clipnear; m_OrthographicFar = clipfar; Recalcualte(); }

		float GetAspectratio() const { return m_Aspectratio; }

		void SetPerspectiveFOV(float fov) { m_PerspectiveFOV = DirectX::XMConvertToRadians(fov); Recalcualte(); }
		void SetPerspectiveNear(float clipnear) { m_PerspectiveNear = clipnear; Recalcualte(); }
		void SetPerspectiveFar(float clipfar) { m_PerspectiveFar = clipfar; Recalcualte(); }
		void SetPerspectiveClip(float clipnear, float clipfar) { m_PerspectiveNear = clipnear; m_PerspectiveFar = clipfar; Recalcualte(); }

		float GetPerspectiveFOV() const { return DirectX::XMConvertToDegrees(m_PerspectiveFOV); }
		float GetPerspectiveNear() const { return m_PerspectiveNear; }
		float GetPerspectiveFar() const { return m_PerspectiveFar; }
		DirectX::XMFLOAT2 GetPerspetiveClip() const { return { m_PerspectiveNear, m_PerspectiveFar }; }

		
		void SetOrthographicZoom(float zoom) { m_OrthographicZoom = zoom; Recalcualte(); }
		void SetOrthographicNear(float clipnear) { m_OrthographicNear = clipnear; Recalcualte(); }
		void SetOrthographicFar(float clipfar) { m_OrthographicFar = clipfar; Recalcualte(); }
		void SetOrthographicClip(float clipnear, float clipfar) { m_OrthographicNear = clipnear; m_OrthographicFar = clipfar; Recalcualte(); }

		float GetOrthographicZoom() const { return m_OrthographicZoom; }
		float GetOrthographicNear() const { return m_OrthographicNear; }
		float GetOrthographicFar() const { return m_OrthographicFar; }
		DirectX::XMFLOAT2 GetOrthographicClip() const { return { m_OrthographicNear, m_OrthographicFar }; }

		void Resize(float width, float height);
	private:
		void Recalcualte();
		void RecaluclatePerspetive();
		void RecaluclateOrthographic();
	private:
		Projection m_ProjectionType = Projection::Perspective;

		float m_Aspectratio = 1.77778f;

		float m_PerspectiveFOV = 0.785398f;
		float m_PerspectiveNear = 0.01f, m_PerspectiveFar = 1000.0f;

		float m_OrthographicZoom = 10.0f;
		float m_OrthographicNear = -1.0f, m_OrthographicFar = 1.0f;
	};

}