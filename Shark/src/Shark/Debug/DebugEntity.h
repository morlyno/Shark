#pragma once

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

namespace Shark {

	class DebugEntity : public Entity
	{
	public:
		DebugEntity(Entity parent)
			: Entity(parent)
		{
			m_TagComponent = TryGetComponent<TagComponent>();
			m_TransformComponent = TryGetComponent<TransformComponent>();
			m_SpriteRendererComponent = TryGetComponent<SpriteRendererComponent>();
			m_CameraComponent = TryGetComponent<CameraComponent>();
			m_NativeScriptComponent = TryGetComponent<NativeScriptComponent>();
			m_RigidBody2DComponent = TryGetComponent<RigidBody2DComponent>();
			m_BoxCollider2DComponent = TryGetComponent<BoxCollider2DComponent>();
		}

	public:
		TagComponent* m_TagComponent = nullptr;
		TransformComponent* m_TransformComponent = nullptr;
		SpriteRendererComponent* m_SpriteRendererComponent = nullptr;
		CameraComponent* m_CameraComponent = nullptr;
		NativeScriptComponent* m_NativeScriptComponent = nullptr;
		RigidBody2DComponent* m_RigidBody2DComponent = nullptr;
		BoxCollider2DComponent* m_BoxCollider2DComponent = nullptr;

	};

}
