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

			ForEachIndexed(AllComponents{}, [this, entity]<typename T, size_t Index>()
			{
				std::get<Index>(m_Components) = entity.TryGetComponent<T>();
			});

		}

		EntityView(entt::entity entity, const entt::registry& registry)
		{
			ForEachIndexed(AllComponents{}, [this, entity, &registry]<typename T, size_t Index>()
			{
				std::get<Index>(m_Components) = registry.try_get<T>(entity);
			});
		}

	public:
		AllComponents::AsConstPointerTuple m_Components;

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
			if (auto* prefabComponent          = entity.TryGetComponent<PrefabComponent>())           m_PrefabComponent = *prefabComponent;
			if (auto* spriteRendererComponent  = entity.TryGetComponent<SpriteRendererComponent>())   m_SpriteRendererComponent = *spriteRendererComponent;
			if (auto* cameraComponent          = entity.TryGetComponent<CameraComponent>())           m_CameraComponent = *cameraComponent;
			if (auto* rigidBody2DComponent     = entity.TryGetComponent<RigidBody2DComponent>())      m_RigidBody2DComponent = *rigidBody2DComponent;
			if (auto* boxCollider2DComponent   = entity.TryGetComponent<BoxCollider2DComponent>())    m_BoxCollider2DComponent = *boxCollider2DComponent;
			if (auto* frictionJointComponent   = entity.TryGetComponent<DistanceJointComponent>())    m_FrictionJointComponent = *frictionJointComponent;
			if (auto* hingeJointComponent   = entity.TryGetComponent<HingeJointComponent>())          m_HingeJointComponent = *hingeJointComponent;
			if (auto* scriptComponent          = entity.TryGetComponent<ScriptComponent>())           m_ScriptComponent = *scriptComponent;

		}

		EntityClone(entt::entity entity, const entt::registry& registry)
		{
			if (auto* iDComponent              = registry.try_get<IDComponent>(entity))               m_IDComponent = *iDComponent;
			if (auto* tagComponent             = registry.try_get<TagComponent>(entity))              m_TagComponent = *tagComponent;
			if (auto* transformComponent       = registry.try_get<TransformComponent>(entity))        m_TransformComponent = *transformComponent;
			if (auto* relationshipComponent    = registry.try_get<RelationshipComponent>(entity))     m_RelationshipComponent = *relationshipComponent;
			if (auto* prefabComponent          = registry.try_get<PrefabComponent>(entity))           m_PrefabComponent = *prefabComponent;
			if (auto* spriteRendererComponent  = registry.try_get<SpriteRendererComponent>(entity))   m_SpriteRendererComponent = *spriteRendererComponent;
			if (auto* cameraComponent          = registry.try_get<CameraComponent>(entity))           m_CameraComponent = *cameraComponent;
			if (auto* rigidBody2DComponent     = registry.try_get<RigidBody2DComponent>(entity))      m_RigidBody2DComponent = *rigidBody2DComponent;
			if (auto* boxCollider2DComponent   = registry.try_get<BoxCollider2DComponent>(entity))    m_BoxCollider2DComponent = *boxCollider2DComponent;
			if (auto* frictionJointComponent   = registry.try_get<DistanceJointComponent>(entity))    m_FrictionJointComponent = *frictionJointComponent;
			if (auto* hingeJointComponent      = registry.try_get<HingeJointComponent>(entity))       m_HingeJointComponent = *hingeJointComponent;
			if (auto* scriptComponent          = registry.try_get<ScriptComponent>(entity))           m_ScriptComponent = *scriptComponent;
		}

	public:
		std::optional<IDComponent> m_IDComponent;
		std::optional<TagComponent> m_TagComponent;
		std::optional<TransformComponent> m_TransformComponent;
		std::optional<RelationshipComponent> m_RelationshipComponent;
		std::optional<PrefabComponent> m_PrefabComponent;
		std::optional<SpriteRendererComponent> m_SpriteRendererComponent;
		std::optional<CameraComponent> m_CameraComponent;
		std::optional<RigidBody2DComponent> m_RigidBody2DComponent;
		std::optional<BoxCollider2DComponent> m_BoxCollider2DComponent;
		std::optional<DistanceJointComponent> m_FrictionJointComponent;
		std::optional<HingeJointComponent> m_HingeJointComponent;
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
			for (auto [ent] : reg.storage<entt::entity>()->each())
				m_Entitys.emplace_back(ent, reg);

		}
	private:
		std::vector<EntityView> m_Entitys;
	};

}

#if SK_DEBUG
#define DEBUG_ENTITY(entity) ::Shark::Debug::EntityView SK_UNIQUE_NAME (entity);
#define DEBUG_ENTITY_N(_var, _entity) ::Shark::Debug::EntityView _var (_entity);
#else
#define DEBUG_ENTITY(...)
#define DEBUG_ENTITY_N(...) (void)0
#endif