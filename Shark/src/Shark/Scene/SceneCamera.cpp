#include "skpch.h"
#include "SceneCamera.h"
#include "Shark/Math/Math.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Shark {

	SceneCamera::SceneCamera()
	{
		Recalcualte();
	}

	SceneCamera::SceneCamera(const Camera& camera)
		: Camera(camera)
	{
	}

	SceneCamera::SceneCamera(const glm::mat4& projection)
		: Camera(projection)
	{
	}

	SceneCamera::SceneCamera(float aspectratio, const PerspectiveSpecs& specs)
		: m_ProjectionType(Projection::Perspective), m_Aspectratio(aspectratio), m_PerspectiveFOV(glm::radians(specs.FOV)), m_PerspectiveNear(specs.Near), m_PerspectiveFar(specs.Far)
	{
		Recalcualte();
	}

	SceneCamera::SceneCamera(float aspectratio, const OrthographicSpecs& specs)
		: m_ProjectionType(Projection::Orthographic), m_Aspectratio(aspectratio), m_OrthographicZoom(specs.Zoom), m_OrthographicNear(specs.Near), m_OrthographicFar(specs.Far)
	{
	}

	SceneCamera::SceneCamera(Projection projection, float aspectratio, const PerspectiveSpecs& ps, const OrthographicSpecs& os)
		: m_ProjectionType(projection), m_Aspectratio(aspectratio),
		  m_PerspectiveFOV(glm::radians(ps.FOV)), m_PerspectiveNear(ps.Near), m_PerspectiveFar(ps.Far),
		  m_OrthographicZoom(os.Zoom), m_OrthographicNear(os.Near), m_OrthographicFar(os.Far)
	{
		Recalcualte();
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
		m_Projection = glm::perspective(
			m_PerspectiveFOV,
			m_Aspectratio,
			m_PerspectiveNear,
			m_PerspectiveFar
		);
	}

	void SceneCamera::RecaluclateOrthographic()
	{
		m_Projection = glm::ortho(
			-m_OrthographicZoom * m_Aspectratio,
			m_OrthographicZoom * m_Aspectratio,
			-m_OrthographicZoom,
			m_OrthographicZoom,
			m_OrthographicNear,
			m_OrthographicFar
		);
	}

}