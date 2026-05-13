#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace Shark {

	class Camera
	{
	public:
		Camera() = default;
		Camera(const glm::mat4& projection) : m_ProjectionMatrix(projection) {}
		virtual ~Camera() = default;

		const glm::mat4& GetProjection() const { return m_ProjectionMatrix; }

		void SetProjectionMatrix(const glm::mat4& projection)
		{
			m_ProjectionMatrix = projection;
		}

		void SetPerspectiveProjectionMatrix(float radFov, float width, float height, float nearClip, float farClip)
		{
			m_ProjectionMatrix = glm::perspectiveFov(radFov, width, height, nearClip, farClip);
		}
		
		void SetPerspectiveProjectionMatrix(float radFov, float aspectRatio, float nearClip, float farClip)
		{
			m_ProjectionMatrix = glm::perspective(radFov, aspectRatio, nearClip, farClip);
		}

	private:
		glm::mat4 m_ProjectionMatrix = glm::mat4(1.0f);

	};

}