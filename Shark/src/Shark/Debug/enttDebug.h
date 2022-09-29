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
			if (!entity)
				return;

			m_IDComponent                = entity.TryGetComponent<IDComponent>();
			m_TagComponent               = entity.TryGetComponent<TagComponent>();
			m_TransformComponent         = entity.TryGetComponent<TransformComponent>();
			m_RelationshipComponent      = entity.TryGetComponent<RelationshipComponent>();
			m_SpriteRendererComponent    = entity.TryGetComponent<SpriteRendererComponent>();
			m_CameraComponent            = entity.TryGetComponent<CameraComponent>();
			m_RigidBody2DComponent       = entity.TryGetComponent<RigidBody2DComponent>();
			m_BoxCollider2DComponent     = entity.TryGetComponent<BoxCollider2DComponent>();
			m_ScriptComponent            = entity.TryGetComponent<ScriptComponent>();
			
		}

		EntityView(entt::entity entity, const entt::registry& registry)
		{
			m_IDComponent                = registry.try_get<IDComponent>(entity);
			m_TagComponent               = registry.try_get<TagComponent>(entity);
			m_TransformComponent         = registry.try_get<TransformComponent>(entity);
			m_RelationshipComponent      = registry.try_get<RelationshipComponent>(entity);
			m_SpriteRendererComponent    = registry.try_get<SpriteRendererComponent>(entity);
			m_CameraComponent            = registry.try_get<CameraComponent>(entity);
			m_RigidBody2DComponent       = registry.try_get<RigidBody2DComponent>(entity);
			m_BoxCollider2DComponent     = registry.try_get<BoxCollider2DComponent>(entity);
			m_ScriptComponent            = registry.try_get<ScriptComponent>(entity);
		}

	public:
		const IDComponent*                     m_IDComponent               = nullptr;
		const TagComponent*                    m_TagComponent              = nullptr;
		const TransformComponent*              m_TransformComponent        = nullptr;
		const RelationshipComponent*           m_RelationshipComponent     = nullptr;
		const SpriteRendererComponent*         m_SpriteRendererComponent   = nullptr;
		const CameraComponent*                 m_CameraComponent           = nullptr;
		const RigidBody2DComponent*            m_RigidBody2DComponent      = nullptr;
		const BoxCollider2DComponent*          m_BoxCollider2DComponent    = nullptr;
		const ScriptComponent*                 m_ScriptComponent           = nullptr;

	};

	class EntityClone
	{
	public:
		EntityClone(Entity entity)
		{
			if (auto* iDComponent              = entity.TryGetComponent<IDComponent>())               m_IDComponent = *iDComponent;
			if (auto* tagComponent             = entity.TryGetComponent<TagComponent>())              m_TagComponent = *tagComponent;
			if (auto* transformComponent       = entity.TryGetComponent<TransformComponent>())        m_TransformComponent = *transformComponent;
			if (auto* relationshipComponent    = entity.TryGetComponent<RelationshipComponent>())     m_RelationshipComponent = *relationshipComponent;
			if (auto* spriteRendererComponent  = entity.TryGetComponent<SpriteRendererComponent>())   m_SpriteRendererComponent = *spriteRendererComponent;
			if (auto* cameraComponent          = entity.TryGetComponent<CameraComponent>())           m_CameraComponent = *cameraComponent;
			if (auto* rigidBody2DComponent     = entity.TryGetComponent<RigidBody2DComponent>())      m_RigidBody2DComponent = *rigidBody2DComponent;
			if (auto* boxCollider2DComponent   = entity.TryGetComponent<BoxCollider2DComponent>())    m_BoxCollider2DComponent = *boxCollider2DComponent;
			if (auto* scriptComponent          = entity.TryGetComponent<ScriptComponent>())           m_ScriptComponent = *scriptComponent;

		}

		EntityClone(entt::entity entity, const entt::registry& registry)
		{
			if (auto* iDComponent              = registry.try_get<IDComponent>(entity))               m_IDComponent = *iDComponent;
			if (auto* tagComponent             = registry.try_get<TagComponent>(entity))              m_TagComponent = *tagComponent;
			if (auto* transformComponent       = registry.try_get<TransformComponent>(entity))        m_TransformComponent = *transformComponent;
			if (auto* relationshipComponent    = registry.try_get<RelationshipComponent>(entity))     m_RelationshipComponent = *relationshipComponent;
			if (auto* spriteRendererComponent  = registry.try_get<SpriteRendererComponent>(entity))   m_SpriteRendererComponent = *spriteRendererComponent;
			if (auto* cameraComponent          = registry.try_get<CameraComponent>(entity))           m_CameraComponent = *cameraComponent;
			if (auto* rigidBody2DComponent     = registry.try_get<RigidBody2DComponent>(entity))      m_RigidBody2DComponent = *rigidBody2DComponent;
			if (auto* boxCollider2DComponent   = registry.try_get<BoxCollider2DComponent>(entity))    m_BoxCollider2DComponent = *boxCollider2DComponent;
			if (auto* scriptComponent          = registry.try_get<ScriptComponent>(entity))           m_ScriptComponent = *scriptComponent;
		}

	public:
		std::optional<IDComponent> m_IDComponent;
		std::optional<TagComponent> m_TagComponent;
		std::optional<TransformComponent> m_TransformComponent;
		std::optional<RelationshipComponent> m_RelationshipComponent;
		std::optional<SpriteRendererComponent> m_SpriteRendererComponent;
		std::optional<CameraComponent> m_CameraComponent;
		std::optional<RigidBody2DComponent> m_RigidBody2DComponent;
		std::optional<BoxCollider2DComponent> m_BoxCollider2DComponent;
		std::optional<ScriptComponent> m_ScriptComponent;
	};

	class SceneView
	{
	public:
		SceneView(Ref<Scene> scene)
		{
			auto view = scene->GetAllEntitysWith<IDComponent>();
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

#if SK_DEBUG
#define DEBUG_ENTITY(entity) ::Shark::Debug::EntityView SK_UNIQUE_NAME (entity);
#else
#define DEBUG_ENTITY(...)
#endif