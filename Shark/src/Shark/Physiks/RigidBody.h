#pragma once

#include "Shark/Core/Base.h"

#include <box2d/box2d.h>
#include <DirectXMath.h>

#include "Shark/Physiks/Collider.h"

namespace Shark {

	enum class BodyType { Static = b2_staticBody, Dynamic = b2_dynamicBody };

	struct RigidBodySpecs
	{
		BodyType Type = BodyType::Static;

		bool AllowSleep = true;
		bool Awake = true;
		bool Enabled = true;
		bool FixedRotation = false;

		DirectX::XMFLOAT2 Position = { 0.0f, 0.0f };
		float Angle = 0.0f;
	};

	class RigidBody
	{
	public:
		RigidBody() = default;
		RigidBody(b2Body* body);

		RigidBody(const RigidBody& other) = default;
		RigidBody& operator=(const RigidBody& other) = default;

		RigidBody(RigidBody&& other) = default;
		RigidBody& operator=(RigidBody&& other) = default;

		BoxCollider CreateBoxCollider(const ColliderSpecs& specs = ColliderSpecs{});
		void DestroyBoxCollider(BoxCollider& collider);

		BodyType GetType() const { return (BodyType)m_Body->GetType(); }
		void SetType(BodyType type) { m_Body->SetType((b2BodyType)type); }

		DirectX::XMFLOAT2 GetPosition() const { auto [x, y] = m_Body->GetPosition(); return { x, y }; }
		float GetAngle() const { return m_Body->GetAngle(); }

		void SetTransform(float x, float y, float angle) { m_Body->SetTransform({ x, y }, angle); }
		void SetPosition(float x, float y) { m_Body->SetTransform({ x, y }, m_Body->GetAngle()); }
		void SetAngle(float angle) { m_Body->SetTransform(m_Body->GetPosition(), angle); }

		bool IsSleepingAllowed() const { return m_Body->IsSleepingAllowed(); }
		bool IsEnabled() const { return m_Body->IsEnabled(); }
		bool IsAwake() const { return m_Body->IsAwake(); }
		bool IsFixedRoation() const { return m_Body->IsFixedRotation(); }

		void SetSleepingAllowed(bool allowed) { m_Body->SetSleepingAllowed(allowed); }
		void SetEnabled(bool enabled) { m_Body->SetEnabled(enabled); }
		void SetAwake(bool awake) { m_Body->SetAwake(awake); }
		void SetFixedRotation(bool fixed) { m_Body->SetFixedRotation(fixed); }

		RigidBodySpecs GetCurrentState() const;
		void SetState(const RigidBodySpecs& specs);

		operator b2Body* () const { return m_Body; }
	private:
		b2Body* m_Body = nullptr;
		uint32_t m_ColliderCount = 0;
	};


}
