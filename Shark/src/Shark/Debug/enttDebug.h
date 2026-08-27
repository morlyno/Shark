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

			Components::All::EachIndexed([this, entity]<typename T, size_t Index>()
			{
				std::get<Index>(m_Components) = entity.TryGetComponent<T>();
			});
		}

		EntityView(entt::entity entity, const entt::registry& registry)
		{
			Components::All::EachIndexed([this, entity, &registry]<typename T, size_t Index>()
			{
				std::get<Index>(m_Components) = registry.try_get<T>(entity);
			});
		}

	public:
		Components::All::AsConstPointerTuple m_Components;

	};

	class EntityClone
	{
	public:
		EntityClone(Entity entity)
		{
			Components::All::EachIndexed([this, entity]<typename T, size_t Index>()
			{
				if (auto* component = entity.TryGetComponent<T>())
					std::get<Index>(m_Components) = *component;
			});
		}

		EntityClone(entt::entity entity, const entt::registry& registry)
		{
			Components::All::EachIndexed([this, entity, &registry]<typename T, size_t Index>()
			{
				if (auto* component = registry.try_get<T>(entity))
					std::get<Index>(m_Components) = *component;
			});
		}

	public:
		Components::All::As<std::optional> m_Components;
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
