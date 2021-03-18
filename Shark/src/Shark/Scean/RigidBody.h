#pragma once

#include "Shark/Core/Base.h"

#include <box2d/box2d.h>
#include <DirectXMath.h>

namespace Shark {

	enum class BodyType { Static = b2_staticBody, Dynamic = b2_dynamicBody };
	enum class ShapeType { Quad };

	struct RigidBodySpecs
	{
		BodyType Type = BodyType::Static;
		bool AllowSleep = true;
		bool Awake = true;
		bool Enabled = true;
		bool FixedRotation = false;

		ShapeType Shape = ShapeType::Quad;
		float Friction = 0.0f;
		float Density = 0.0f;
		float Restitution = 0.0f;

		DirectX::XMFLOAT2 Position = { 0.0f, 0.0f };
		float Angle = 0.0f;
		DirectX::XMFLOAT2 Size = { 1.0f, 1.0f };
	};

	class RigidBody
	{
	public:
		RigidBody() = default;
		RigidBody(b2Body* body, float width, float height);
		RigidBody(const RigidBody& other);
		~RigidBody() = default;

		void Resize(float width, float height);

		BodyType GetType() const { return (BodyType)m_Body->GetType(); }
		void SetType(BodyType type) { m_Body->SetType((b2BodyType)type); }

		ShapeType GetShape() const { return ShapeType::Quad; }
		void SetShape(...) { SK_CORE_ASSERT(false, "Shape can not be changed at the moment"); }

		DirectX::XMFLOAT2 GetPosition() const { auto [x, y] = m_Body->GetPosition(); return { x, y }; }
		float GetAngle() const { return m_Body->GetAngle(); }
		DirectX::XMFLOAT2 GetSize() const { return { m_Width, m_Height }; }

		void SetTransform(float x, float y, float angle) { m_Body->SetTransform({ x, y }, angle); }
		void SetPosition(float x, float y) { m_Body->SetTransform({ x, y }, m_Body->GetAngle()); }
		void SetAngle(float angle) { m_Body->SetTransform(m_Body->GetPosition(), angle); }


		float GetFriction() const { return m_Body->GetFixtureList()->GetFriction(); }
		float GetDensity() const { return m_Body->GetFixtureList()->GetDensity(); }
		float GetRestituion() const { return m_Body->GetFixtureList()->GetRestitution(); }

		void SetFriction(float friction) { m_Body->GetFixtureList()->SetFriction(friction); }
		void SetDensity(float density) { m_Body->GetFixtureList()->SetDensity(density); m_Body->ResetMassData(); }
		void SetRestitution(float restituion) { m_Body->GetFixtureList()->SetRestitution(restituion); }


		bool IsSleepingAllowed() const { return m_Body->IsSleepingAllowed(); }
		bool IsEnabled() const { return m_Body->IsEnabled(); }
		bool IsAwake() const { return m_Body->IsAwake(); }
		bool IsFixedRoation() const { return m_Body->IsFixedRotation(); }

		void SetSleepingAllowed(bool allowed) { m_Body->SetSleepingAllowed(allowed); }
		void SetEnabled(bool enabled) { m_Body->SetEnabled(enabled); }
		void SetAwake(bool awake) { m_Body->SetAwake(awake); }
		void SetFixedRotation(bool fixed) { m_Body->SetFixedRotation(fixed); }

		RigidBodySpecs GetCurrentState() const;
		void SetState(const RigidBodySpecs& state);

		operator b2Body* () { return m_Body; }
	private:
		b2Body* m_Body = nullptr;
		float m_Width = 0.0f, m_Height = 0.0f;
	};

	class World
	{
	public:
		World() : m_World({ 0.0f, -10.0f }) {}
		~World() {};
		
		void Update(float timeStep = 1.0f / 60.0f) { m_World.Step(timeStep, 8, 3); }
		void Flush()
		{
			b2Body* bodylist = m_World.GetBodyList();
			while (bodylist)
			{
				bodylist->SetLinearVelocity({ 0.0f, 0.0f });
				bodylist->SetAngularVelocity(0.0f);

				bodylist = bodylist->GetNext();
			}
		}

		RigidBody CreateBody(const RigidBodySpecs& specs = RigidBodySpecs{})
		{
			b2BodyDef bodydef;
			bodydef.type = (b2BodyType)specs.Type;
			bodydef.position = { specs.Position.x, specs.Position.y };
			bodydef.angle = specs.Angle;
			bodydef.allowSleep = specs.AllowSleep;
			bodydef.awake = specs.Awake;
			bodydef.enabled = specs.Enabled;
			bodydef.fixedRotation = specs.FixedRotation;

			b2Body* body = m_World.CreateBody(&bodydef);

			b2PolygonShape shape;
			shape.SetAsBox(specs.Size.x * 0.5f, specs.Size.y * 0.5f);

			b2FixtureDef fixturedef;
			fixturedef.shape = &shape;
			fixturedef.friction = specs.Friction;
			fixturedef.density = specs.Density;
			fixturedef.restitution = specs.Restitution;

			body->CreateFixture(&fixturedef);

			RigidBody rigidbody{ body, specs.Size.x, specs.Size.y };
			return rigidbody;
		};
		void DestroyBody(RigidBody& body) { m_World.DestroyBody(body); }
	private:
		b2World m_World;
	};


}
