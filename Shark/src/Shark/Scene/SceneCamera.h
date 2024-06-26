#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Render/Camera.h"

#include <glm/glm.hpp>

namespace Shark {

	class SceneCamera : public Camera
	{
	public:
		struct PerspectiveSpecs
		{
			float FOV = 0.785398f;
			float Near = 0.01f;
			float Far = 1000.0f;

			PerspectiveSpecs() = default;
			PerspectiveSpecs(float aspectratio, float fov, float clipNear, float clipFar)
				: FOV(fov), Near(clipNear), Far(clipFar) {}
		};
		struct OrthographicSpecs
		{
			float Zoom = 10.0f;
			float Near = -1.0f;
			float Far = 1.0f;

			OrthographicSpecs() = default;
			OrthographicSpecs(float aspectratio, float zoom, float clipNear, float clipFar)
				: Zoom(zoom), Near(clipNear), Far(clipFar) {}
		};

		enum class Projection
		{
			None = 0,
			Perspective, Orthographic
		};
	public:
		SceneCamera();
		SceneCamera(const Camera& camera);
		SceneCamera(const glm::mat4& projection);
		SceneCamera(float aspectratio, const PerspectiveSpecs& specs);
		SceneCamera(float aspectratio, const OrthographicSpecs& specs);
		SceneCamera(Projection projection, float aspectratio, const PerspectiveSpecs& ps, const OrthographicSpecs& os);


		void SetProjectionType(Projection projection) { m_ProjectionType = projection; Recalcualte(); }
		Projection GetProjectionType() const { return m_ProjectionType; }

		void SetPerspective(float aspectratio, float fov, float clipnear, float clipfar) { m_Aspectratio = aspectratio; m_PerspectiveFOV = glm::radians(fov), m_PerspectiveNear = clipnear, m_PerspectiveFar = clipfar; Recalcualte(); }
		void SetOrthographic(float aspectratio, float zoom, float clipnear, float clipfar) { m_Aspectratio = aspectratio; m_OrthographicZoom = zoom; m_OrthographicNear = clipnear; m_OrthographicFar = clipfar; Recalcualte(); }

		float GetAspectratio() const { return m_Aspectratio; }
		float SetAspectratio(float aspectratio) { return m_Aspectratio = aspectratio; Recalcualte(); }

		void SetPerspectiveFOV(float fov) { m_PerspectiveFOV = glm::radians(fov); Recalcualte(); }
		void SetPerspectiveNear(float clipnear) { m_PerspectiveNear = clipnear; Recalcualte(); }
		void SetPerspectiveFar(float clipfar) { m_PerspectiveFar = clipfar; Recalcualte(); }
		void SetPerspectiveClip(float clipnear, float clipfar) { m_PerspectiveNear = clipnear; m_PerspectiveFar = clipfar; Recalcualte(); }

		float GetPerspectiveFOV() const { return glm::degrees(m_PerspectiveFOV); }
		float GetRadPerspectiveFOV() const { return m_PerspectiveFOV; }
		float GetPerspectiveNear() const { return m_PerspectiveNear; }
		float GetPerspectiveFar() const { return m_PerspectiveFar; }
		glm::vec2 GetPerspetiveClip() const { return { m_PerspectiveNear, m_PerspectiveFar }; }

		
		void SetOrthographicZoom(float zoom) { m_OrthographicZoom = zoom; Recalcualte(); }
		void SetOrthographicNear(float clipnear) { m_OrthographicNear = clipnear; Recalcualte(); }
		void SetOrthographicFar(float clipfar) { m_OrthographicFar = clipfar; Recalcualte(); }
		void SetOrthographicClip(float clipnear, float clipfar) { m_OrthographicNear = clipnear; m_OrthographicFar = clipfar; Recalcualte(); }

		float GetOrthographicZoom() const { return m_OrthographicZoom; }
		float GetOrthographicNear() const { return m_OrthographicNear; }
		float GetOrthographicFar() const { return m_OrthographicFar; }
		glm::vec2 GetOrthographicClip() const { return { m_OrthographicNear, m_OrthographicFar }; }

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

	constexpr std::string_view ToStringView(SceneCamera::Projection projection)
	{
		switch (projection)
		{
			case SceneCamera::Projection::None:          return "None"sv;
			case SceneCamera::Projection::Perspective:   return "Perspective"sv;
			case SceneCamera::Projection::Orthographic:  return "Orthographic"sv;
		}

		SK_CORE_ASSERT(false, "Unkown SceneCamera::Projection");
		return std::string_view{};
	}

	constexpr SceneCamera::Projection StringToSceneCameraProjection(std::string_view projection)
	{
		if (projection == "None") return SceneCamera::Projection::None;
		if (projection == "Perspective") return SceneCamera::Projection::Perspective;
		if (projection == "Orthographic") return SceneCamera::Projection::Orthographic;

		SK_CORE_ASSERT(false, "Unkonw SceneCamera::Projection string");
		return SceneCamera::Projection::None;
	}

}