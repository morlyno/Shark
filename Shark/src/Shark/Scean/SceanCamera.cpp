#include "skpch.h"
#include "SceanCamera.h"

namespace Shark {

	SceanCamera::SceanCamera()
	{
		Recalcualte();
	}

	SceanCamera::SceanCamera(const Camera& camera)
		: Camera(camera)
	{
	}

	SceanCamera::SceanCamera(const DirectX::XMMATRIX& projection)
		: Camera(projection)
	{
	}

	void SceanCamera::Resize(float width, float height)
	{
		m_Aspectratio = width / height;

		Recalcualte();
	}

	void SceanCamera::Recalcualte()
	{
		if (m_ProjectionType == Projection::Perspective)
			RecaluclatePerspetive();
		else
			RecaluclateOrthographic();
	}

	void SceanCamera::RecaluclatePerspetive()
	{
		m_Projection = DirectX::XMMatrixPerspectiveFovLH(m_PerspectiveFOV, m_Aspectratio, m_PerspectiveNear, m_PerspectiveFar);
	}

	void SceanCamera::RecaluclateOrthographic()
	{
		m_Projection = DirectX::XMMatrixOrthographicOffCenterLH(-m_OrthographicZoom * m_Aspectratio, m_OrthographicZoom * m_Aspectratio, -m_OrthographicZoom, m_OrthographicZoom, m_OrthographicNear, m_OrthographicFar);
	}

}