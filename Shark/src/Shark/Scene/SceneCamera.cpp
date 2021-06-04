#include "skpch.h"
#include "SceneCamera.h"

namespace Shark {

	SceneCamera::SceneCamera()
	{
		Recalcualte();
	}

	SceneCamera::SceneCamera(const Camera& camera)
		: Camera(camera)
	{
	}

	SceneCamera::SceneCamera(const DirectX::XMMATRIX& projection)
		: Camera(projection)
	{
	}

	void SceneCamera::Resize(float width, float height)
	{
		m_Aspectratio = width / height;

		Recalcualte();
	}

	void SceneCamera::Recalcualte()
	{
		if (m_ProjectionType == Projection::Perspective)
			RecaluclatePerspetive();
		else
			RecaluclateOrthographic();
	}

	void SceneCamera::RecaluclatePerspetive()
	{
		m_Projection = DirectX::XMMatrixPerspectiveFovLH(m_PerspectiveFOV, m_Aspectratio, m_PerspectiveNear, m_PerspectiveFar);
	}

	void SceneCamera::RecaluclateOrthographic()
	{
		m_Projection = DirectX::XMMatrixOrthographicOffCenterLH(-m_OrthographicZoom * m_Aspectratio, m_OrthographicZoom * m_Aspectratio, -m_OrthographicZoom, m_OrthographicZoom, m_OrthographicNear, m_OrthographicFar);
	}

}