#pragma once

#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

namespace Shark::Debug {

	class DebugEntity
	{
	public:
		DebugEntity(Entity parent)
		{
			m_IDComponent                = parent.TryGetComponent<IDComponent>();
			m_TagComponent               = parent.TryGetComponent<TagComponent>();
			m_TransformComponent         = parent.TryGetComponent<TransformComponent>();
			m_SpriteRendererComponent    = parent.TryGetComponent<SpriteRendererComponent>();
			m_CameraComponent            = parent.TryGetComponent<CameraComponent>();
			m_NativeScriptComponent      = parent.TryGetComponent<NativeScriptComponent>();
			m_RigidBody2DComponent       = parent.TryGetComponent<RigidBody2DComponent>();
			m_BoxCollider2DComponent     = parent.TryGetComponent<BoxCollider2DComponent>();
		}

		DebugEntity(entt::entity entity, const entt::registry& registry)
		{
			m_IDComponent                = registry.try_get<IDComponent>(entity);
			m_TagComponent               = registry.try_get<TagComponent>(entity);
			m_TransformComponent         = registry.try_get<TransformComponent>(entity);
			m_SpriteRendererComponent    = registry.try_get<SpriteRendererComponent>(entity);
			m_CameraComponent            = registry.try_get<CameraComponent>(entity);
			m_NativeScriptComponent      = registry.try_get<NativeScriptComponent>(entity);
			m_RigidBody2DComponent       = registry.try_get<RigidBody2DComponent>(entity);
			m_BoxCollider2DComponent     = registry.try_get<BoxCollider2DComponent>(entity);
		}

	public:
		const IDComponent*                     m_IDComponent               = nullptr;
		const TagComponent*                    m_TagComponent              = nullptr;
		const TransformComponent*              m_TransformComponent        = nullptr;
		const SpriteRendererComponent*         m_SpriteRendererComponent   = nullptr;
		const CameraComponent*                 m_CameraComponent           = nullptr;
		const NativeScriptComponent*           m_NativeScriptComponent     = nullptr;
		const RigidBody2DComponent*            m_RigidBody2DComponent      = nullptr;
		const BoxCollider2DComponent*          m_BoxCollider2DComponent    = nullptr;

	};

	class DebugRegistry
	{
	public:
		DebugRegistry(const entt::registry& reg)
		{
			reg.each([this, &reg](auto e)
			{
				m_Entitys.emplace_back(e, reg);
			});
		}
	private:
		std::vector<DebugEntity> m_Entitys;
	};

}
