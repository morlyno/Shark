#pragma once

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

namespace Shark::Debug {

	class DebugEntity : public Entity
	{
	public:
		DebugEntity(Entity parent)
			: Entity(parent)
		{
			m_IDComponent = TryGetComponent<IDComponent>();
			m_TagComponent = TryGetComponent<TagComponent>();
			m_TransformComponent = TryGetComponent<TransformComponent>();
			m_SpriteRendererComponent = TryGetComponent<SpriteRendererComponent>();
			m_CameraComponent = TryGetComponent<CameraComponent>();
			m_NativeScriptComponent = TryGetComponent<NativeScriptComponent>();
			m_RigidBody2DComponent = TryGetComponent<RigidBody2DComponent>();
			m_BoxCollider2DComponent = TryGetComponent<BoxCollider2DComponent>();
		}

	public:
		IDComponent* m_IDComponent = nullptr;
		TagComponent* m_TagComponent = nullptr;
		TransformComponent* m_TransformComponent = nullptr;
		SpriteRendererComponent* m_SpriteRendererComponent = nullptr;
		CameraComponent* m_CameraComponent = nullptr;
		NativeScriptComponent* m_NativeScriptComponent = nullptr;
		RigidBody2DComponent* m_RigidBody2DComponent = nullptr;
		BoxCollider2DComponent* m_BoxCollider2DComponent = nullptr;

	};

	class DebugRegistry
	{
	public:
		DebugRegistry(const entt::registry& reg, Ref<Scene> scene)
		{
			reg.each([this, &reg, scene](auto e)
			{
				m_Entitys.emplace_back(Entity{ e, scene });
			});
		}
	private:
		std::vector<DebugEntity> m_Entitys;
	};

}
