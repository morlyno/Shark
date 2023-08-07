#pragma once

#include "Shark/Render/Font.h"
#include "Shark/Render/Mesh.h"
#include "Shark/Scene/SceneCamera.h"
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
		UUID ID = UUID::Null;

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

		void SetTransform(const glm::mat4& transform)
		{
			Math::DecomposeTransform(transform, Translation, Rotation, Scale);
		}

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
	};

	struct RelationshipComponent
	{
		UUID Parent = UUID::Null;
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

	struct MeshRendererComponent
	{
		AssetHandle MeshHandle;
		uint32_t SubmeshIndex;
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

	struct DistanceJointComponent
	{
		UUID ConnectedEntity = UUID::Null;
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
		UUID ConnectedEntity = UUID::Null;
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
		UUID ConnectedEntity = UUID::Null;
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
		UUID ConnectedEntity = UUID::Null;
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

	inline std::string_view ToStringView(RigidBody2DComponent::BodyType type)
	{
		switch (type)
		{
			case RigidBody2DComponent::BodyType::None: return "None";
			case RigidBody2DComponent::BodyType::Static: return "Static";
			case RigidBody2DComponent::BodyType::Dynamic: return "Dynamic";
			case RigidBody2DComponent::BodyType::Kinematic: return "Kinematic";
		}

		SK_CORE_ASSERT(false, "Unkown RigidBody2DType");
		return "Unkown";
	}

	inline RigidBody2DComponent::BodyType StringToRigidBody2DType(std::string_view type)
	{
		if (type == "None") return RigidBody2DComponent::BodyType::None;
		if (type == "Static") return RigidBody2DComponent::BodyType::Static;
		if (type == "Dynamic") return RigidBody2DComponent::BodyType::Dynamic;
		if (type == "Kinematic") return RigidBody2DComponent::BodyType::Kinematic;

		SK_CORE_ASSERT(false, "Unkown RigidBody2DType string");
		return RigidBody2DComponent::BodyType::None;
	}

}
