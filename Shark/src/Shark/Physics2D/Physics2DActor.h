#pragma once

#include "Shark/Core/Base.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

class b2Body;

namespace Shark {

	enum class RigidBody2DType
	{
		None = 0,
		Static,
		Dynamic,
		Kinematic
	};

	class Physics2DActor : public RefCount
	{
	public:
		Physics2DActor() = default;
		~Physics2DActor();

		Entity GetEntity() const;
		b2Body* GetBody() const { return m_Body; }

		void AddCollider(BoxCollider2DComponent& collider);
		void AddCollider(CircleCollider2DComponent& collider);
		void RemoveCollider(BoxCollider2DComponent& collider);
		void RemoveCollider(CircleCollider2DComponent& collider);

		void SetBodyType(RigidBody2DType bodyType);
		RigidBody2DType GetBodyType() const;

		bool IsStatic() const;

	private:
		void SyncronizeTransforms();

	private:
		Entity m_Entity;
		b2Body* m_Body;

		friend class Physics2DScene;
	};

}
