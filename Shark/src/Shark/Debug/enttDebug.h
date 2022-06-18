#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components.h"

namespace Shark::Debug {

	class EntityView
	{
	public:
		EntityView(Entity entity)
		{
			m_IDComponent                = entity.TryGetComponent<IDComponent>();
			m_TagComponent               = entity.TryGetComponent<TagComponent>();
			m_TransformComponent         = entity.TryGetComponent<TransformComponent>();
			m_SpriteRendererComponent    = entity.TryGetComponent<SpriteRendererComponent>();
			m_CameraComponent            = entity.TryGetComponent<CameraComponent>();
			m_NativeScriptComponent      = entity.TryGetComponent<NativeScriptComponent>();
			m_RigidBody2DComponent       = entity.TryGetComponent<RigidBody2DComponent>();
			m_BoxCollider2DComponent     = entity.TryGetComponent<BoxCollider2DComponent>();
			m_ScriptComponent            = entity.TryGetComponent<ScriptComponent>();
			
		}

		EntityView(entt::entity entity, const entt::registry& registry)
		{
			m_IDComponent                = registry.try_get<IDComponent>(entity);
			m_TagComponent               = registry.try_get<TagComponent>(entity);
			m_TransformComponent         = registry.try_get<TransformComponent>(entity);
			m_SpriteRendererComponent    = registry.try_get<SpriteRendererComponent>(entity);
			m_CameraComponent            = registry.try_get<CameraComponent>(entity);
			m_NativeScriptComponent      = registry.try_get<NativeScriptComponent>(entity);
			m_RigidBody2DComponent       = registry.try_get<RigidBody2DComponent>(entity);
			m_BoxCollider2DComponent     = registry.try_get<BoxCollider2DComponent>(entity);
			m_ScriptComponent            = registry.try_get<ScriptComponent>(entity);
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
		const ScriptComponent*                 m_ScriptComponent           = nullptr;

	};

	class SceneView
	{
	public:
		SceneView(Ref<Scene> scene)
		{
			auto view = scene->GetAllEntitysWith<UUID>();
			for (auto entityID : view)
			{
				Entity entity = { entityID, scene };
				m_Entitys.emplace_back(entity);
			}
		}

		SceneView(const entt::registry& reg)
		{
			reg.each([this, &reg](entt::entity e)
			{
				m_Entitys.emplace_back(e, reg);
			});
		}
	private:
		std::vector<EntityView> m_Entitys;
	};

}
