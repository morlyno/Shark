#pragma once

#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Shark {

	struct TransformComponent
	{
		glm::mat4 CalcTransform() const
		{
			return glm::translate(glm::mat4(1), Translation) *
				glm::toMat4(glm::quat(Rotation)) *
				glm::scale(glm::mat4(1), Scale);
		}

		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
	};

	class TransformUtils
	{
	public:
		static void MakeLocal(TransformComponent& transform, const TransformComponent& worldParent);
		static void MakeWorld(TransformComponent& transform, const TransformComponent& worldParent);

		static TransformComponent ToLocal(const TransformComponent& world, const TransformComponent& worldParent);
		static TransformComponent ToWorld(const TransformComponent& local, const TransformComponent& worldParent);
	};

	inline void TransformUtils::MakeLocal(TransformComponent& transform, const TransformComponent& worldParent)
	{
		glm::mat4 worldToLocal = glm::inverse(worldParent.CalcTransform());
		transform.Translation = worldToLocal * glm::vec4(transform.Translation, 1.0f);
		transform.Rotation = transform.Rotation - worldParent.Rotation;
		transform.Scale = transform.Scale / worldParent.Scale;
	}

	inline void TransformUtils::MakeWorld(TransformComponent& transform, const TransformComponent& worldParent)
	{
		glm::mat4 localToWorld = worldParent.CalcTransform();
		transform.Translation = localToWorld * glm::vec4(transform.Translation, 1.0f);
		transform.Rotation += worldParent.Rotation;
		transform.Scale *= worldParent.Scale;
	}

	inline TransformComponent TransformUtils::ToLocal(const TransformComponent& world, const TransformComponent& worldParent)
	{
		// dmat4 fixes the result being off by flt epsilon
		glm::dmat4 worldToLocal = glm::inverse((glm::dmat4)worldParent.CalcTransform());
		return {
			worldToLocal * glm::vec4(world.Translation, 1.0f),
			world.Rotation - worldParent.Rotation,
			world.Scale / worldParent.Scale
		};
	}

	inline TransformComponent TransformUtils::ToWorld(const TransformComponent& local, const TransformComponent& worldParent)
	{
		glm::mat4 localToWorld = worldParent.CalcTransform();
		return {
			localToWorld * glm::vec4(local.Translation, 1.0f),
			local.Rotation + worldParent.Rotation,
			local.Scale * worldParent.Scale
		};
	}

}