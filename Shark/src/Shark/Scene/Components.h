#pragma once

#include "Shark/Scene/SceneCamera.h"
#include "Shark/Scripting/ScriptTypes.h"

#include <glm/glm.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

class b2Body;
class b2Fixture;

namespace Shark {

	struct IDComponent
	{
		UUID ID = UUID::Invalid;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		glm::mat4 CalcTransform() const
		{
			return glm::translate(glm::mat4(1), Translation) *
				glm::toMat4(glm::quat(Rotation)) *
				glm::scale(glm::mat4(1), Scale);
		}

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
	};

	struct RelationshipComponent
	{
		UUID Parent = UUID::Invalid;
		std::vector<UUID> Children;

		RelationshipComponent() = default;
		RelationshipComponent(const RelationshipComponent&) = default;
	};

	struct SpriteRendererComponent
	{
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		AssetHandle TextureHandle;
		float TilingFactor = 1.0f;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
	};

	struct CircleRendererComponent
	{
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.002f;

		CircleRendererComponent() = default;
		CircleRendererComponent(const CircleRendererComponent&) = default;
	};

	struct CameraComponent
	{
		SceneCamera Camera;

		const glm::mat4& GetProjection() const
		{
			return Camera.GetProjection();
		}

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};

	struct RigidBody2DComponent
	{
		enum class BodyType { None = 0, Static, Dynamic, Kinematic };
		BodyType Type = BodyType::Dynamic;
		bool FixedRotation = false;
		bool IsBullet = false;
		bool Awake = true;
		bool Enabled = true;
		bool AllowSleep = true;
		float GravityScale = 1.0f;

		b2Body* RuntimeBody = nullptr;

		RigidBody2DComponent() = default;
		RigidBody2DComponent(const RigidBody2DComponent&) = default;
	};

	struct BoxCollider2DComponent
	{
		glm::vec2 Size = { 0.5f, 0.5f };
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Rotation = 0.0f;

		float Density = 1.0f;
		float Friction = 0.2f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 1.0f;

		bool IsSensor = false;

		b2Fixture* RuntimeCollider;

		BoxCollider2DComponent() = default;
		BoxCollider2DComponent(const BoxCollider2DComponent&) = default;
	};

	struct CircleCollider2DComponent
	{
		float Radius = 0.5f;
		glm::vec2 Offset = { 0.0f, 0.0f };
		float Rotation = 0.0f;

		float Density = 1.0f;
		float Friction = 0.2f;
		float Restitution = 0.0f;
		float RestitutionThreshold = 1.0f;

		bool IsSensor = false;

		b2Fixture* RuntimeCollider;

		CircleCollider2DComponent() = default;
		CircleCollider2DComponent(const CircleCollider2DComponent&) = default;
	};

	class ScriptComponent
	{
	public:
		std::string ScriptName;

	private:
		Ref<ScriptClass> m_Class;

	public:
		Ref<ScriptClass> GetClass() { return m_Class; }

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;

		friend class ScriptEngine;
	};

}
