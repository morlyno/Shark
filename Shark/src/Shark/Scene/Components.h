#pragma once

#include "Shark/Render/Font.h"
#include "Shark/Render/Mesh.h"
#include "Shark/Render/Camera.h"
#include "Shark/Scripting/ScriptTypes.h"
#include "Shark/Math/Math.h"

#include <glm/glm.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

class b2Body;
class b2Fixture;
class b2DistanceJoint;
class b2RevoluteJoint;
class b2PrismaticJoint;
class b2PulleyJoint;

namespace Shark {

	struct IDComponent
	{
		UUID ID = UUID::Invalid;

		IDComponent() = default;
		IDComponent(const IDComponent&) = default;
		IDComponent(UUID id)
			: ID(id) {}
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
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

		void SetTransform(const glm::mat4& transform)
		{
			Math::DecomposeTransform(transform, Translation, Rotation, Scale);
		}

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
			: Translation(translation), Rotation(rotation), Scale(scale) {}
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
		bool Transparent = false;

		SpriteRendererComponent() = default;
		SpriteRendererComponent(const SpriteRendererComponent&) = default;
	};

	struct CircleRendererComponent
	{
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		float Thickness = 1.0f;
		float Fade = 0.002f;
		bool Filled = true;
		bool Transparent = false;

		CircleRendererComponent() = default;
		CircleRendererComponent(const CircleRendererComponent&) = default;
	};

	struct TextRendererComponent
	{
		AssetHandle FontHandle;
		std::string Text;
		glm::vec4 Color = glm::vec4(1.0f);
		float Kerning = 0.0f;
		float LineSpacing = 0.0f;
	};

	struct MeshComponent
	{
		AssetHandle Mesh;
		uint32_t SubmeshIndex = 0;
		AssetHandle Material;
		bool Visible = true;

		MeshComponent() = default;
		MeshComponent(AssetHandle mesh, AssetHandle material = AssetHandle::Invalid)
			: Mesh(mesh), Material(material) {}
	};

	struct PointLightComponent
	{
		glm::vec4 Radiance = glm::vec4(1.0f);
		float Intensity = 1.0f;
		float Radius = 10.0f;
		float Falloff = 1.0f;
	};

	struct DirectionalLightComponent
	{
		glm::vec4 Radiance = glm::vec4(1.0f);
		float Intensity = 1.0f;
	};

	struct SkyComponent
	{
		AssetHandle SceneEnvironment = AssetHandle::Invalid;
		bool DynamicSky = false;
		float Intensity = 1.0f;
		float Lod = 0.0f;

		SkyComponent() = default;
		SkyComponent(AssetHandle environment)
			: SceneEnvironment(environment) {}
	};

	struct CameraComponent
	{
		bool IsPerspective = true;
		float AspectRatio = 16.0f / 9.0f;
		float OrthographicSize = 10.0f;
		float PerspectiveFOV = 0.785398f;
		float Near = 0.3f;
		float Far = 1000.0f;

		Camera Camera;

		const glm::mat4& GetProjection() const { return Camera.GetProjection(); }
		void SetProjection(const glm::mat4& projection) { Camera.SetProjection(projection); }

		void RecalculatePerspective() { SetProjection(glm::perspective(PerspectiveFOV, AspectRatio, Near, Far)); }
		void RecalculateOrthographic() { SetProjection(glm::ortho(-OrthographicSize * AspectRatio, OrthographicSize * AspectRatio, OrthographicSize, OrthographicSize, Near, Far)); }
		void Recalculate() { IsPerspective ? RecalculatePerspective() : RecalculateOrthographic(); }

		CameraComponent() = default;
		CameraComponent(bool perspective) : IsPerspective(perspective) {}
		CameraComponent(bool perspective, float aspectRatio) : IsPerspective(perspective), AspectRatio(aspectRatio) {}
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

	struct DistanceJointComponent
	{
		UUID ConnectedEntity = UUID::Invalid;
		bool CollideConnected = true;

		glm::vec2 AnchorOffsetA;
		glm::vec2 AnchorOffsetB;

		float MinLength = -1.0f;
		float MaxLength = -1.0f;

		float Stiffness = 0.0f;
		float Damping = 0.0f;

		b2DistanceJoint* RuntimeJoint;
	};

	struct HingeJointComponent
	{
		UUID ConnectedEntity = UUID::Invalid;
		bool CollideConnected = true;

		glm::vec2 Anchor = glm::vec2(0.0f);
		float LowerAngle = 0.0f;
		float UpperAngle = 0.0f;

		bool EnableMotor = false;
		float MotorSpeed = 0.0f;
		float MaxMotorTorque = 0.0f;

		b2RevoluteJoint* RuntimeJoint;
	};

	struct PrismaticJointComponent
	{
		UUID ConnectedEntity = UUID::Invalid;
		bool CollideConnected = true;

		glm::vec2 Anchor = glm::vec2(0.0f);
		glm::vec2 Axis = glm::vec2(1.0f, 0.0f);

		bool EnableLimit = false;
		float LowerTranslation = 0.0f;
		float UpperTranslation = 0.0f;

		bool EnableMotor = false;
		float MaxMotorForce = 0.0f;
		float MotorSpeed = 0.0f;

		b2PrismaticJoint* RuntimeJoint;
	};

	struct PulleyJointComponent
	{
		UUID ConnectedEntity = UUID::Invalid;
		bool CollideConnected = true;

		glm::vec2 AnchorA = glm::vec2(0.0f);
		glm::vec2 AnchorB = glm::vec2(0.0f);
		glm::vec2 GroundAnchorA = glm::vec2(0.0f);
		glm::vec2 GroundAnchorB = glm::vec2(0.0f);
		float Ratio = 1.0f;

		b2PulleyJoint* RuntimeJoint;
	};

	struct ScriptComponent
	{
		std::string ScriptName;
		uint64_t ClassID = 0;
		//Ref<ScriptClass> Class;

		ScriptComponent() = default;
		ScriptComponent(const ScriptComponent&) = default;

		friend class ScriptEngine;
	};

	template<typename... TComponents>
	struct ComponentGroup {};

	constexpr auto AllComponents = ComponentGroup<
		// NOTE(moro): NO IDComponent
		TagComponent,
		TransformComponent,
		RelationshipComponent,
		SpriteRendererComponent,
		CircleRendererComponent,
		TextRendererComponent,
		MeshComponent,
		PointLightComponent,
		DirectionalLightComponent,
		SkyComponent,
		CameraComponent,
		RigidBody2DComponent,
		BoxCollider2DComponent,
		CircleCollider2DComponent,
		DistanceJointComponent,
		HingeJointComponent,
		PrismaticJointComponent,
		PulleyJointComponent,
		ScriptComponent
	>{};

	template<typename TFunc, typename... TComponents>
	inline static void ForEach(TFunc func)
	{
		([&]()
		{
			func.template operator()<TComponents>();
		}(), ...);
	}

	template<typename TFunc, typename... TComponents>
	inline static void ForEach(ComponentGroup<TComponents...> componentGroup, TFunc func)
	{
		ForEach<TFunc, TComponents...>(func);
	}

}
