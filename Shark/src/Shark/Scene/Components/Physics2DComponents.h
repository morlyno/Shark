#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"

#include "Shark/Physics2D/PhysicsTypes.h"

class b2Body;
class b2Fixture;
class b2DistanceJoint;
class b2RevoluteJoint;
class b2PrismaticJoint;
class b2PulleyJoint;

namespace Shark {

	struct RigidBody2DComponent
	{
		RigidbodyType Type = RigidbodyType::Dynamic;
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

		b2Fixture* RuntimeCollider = nullptr;

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

		b2Fixture* RuntimeCollider = nullptr;

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

		b2DistanceJoint* RuntimeJoint = nullptr;
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

		b2RevoluteJoint* RuntimeJoint = nullptr;
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

		b2PrismaticJoint* RuntimeJoint = nullptr;
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

		b2PulleyJoint* RuntimeJoint = nullptr;
	};

}
