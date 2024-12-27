#pragma once

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Components.h"

#include <entt.hpp>

namespace Shark {

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity entityhandle, Weak<Scene> scene);

		bool IsValid() const;

		template<typename TComponent, typename... TArgs>
		TComponent& AddComponent(TArgs&&... args);

		template<typename TComponent, typename... TArgs>
		TComponent& AddOrReplaceComponent(TArgs&&... args);

		template<typename TComponent>
		TComponent& GetComponent();

		template<typename TComponent>
		const TComponent& GetComponent() const;

		template<typename TComponent>
		TComponent* TryGetComponent();
		
		template<typename TComponent>
		const TComponent* TryGetComponent() const;

		template<typename TComponent>
		void RemoveComponent();

		template<typename TComponent>
		void RemoveComponentIsExists();

		template<typename TComponent>
		bool HasComponent() const;

		template<typename... TComponents>
		bool HasAny() const;

		template<typename... TComponents>
		bool HasAll() const;

		Entity Parent() const;
		UUID ParentID() const;
		std::vector<UUID>& Children();
		const std::vector<UUID>& Children() const;

		bool HasParent() const;
		bool HasChild(UUID childID) const;
		bool HasChildren() const;

		void SetParent(Entity parent);
		void AddChild(Entity child);
		void RemoveChild(Entity child);

		bool IsAncestorOf(Entity entity) const;
		bool IsDescendantOf(Entity entity) const;

		UUID GetUUID() const { return GetComponent<IDComponent>().ID; }
		std::string& Tag() { return GetComponent<TagComponent>().Tag; }
		const std::string& Tag() const { return GetComponent<TagComponent>().Tag; }
		TransformComponent& Transform() { return GetComponent<TransformComponent>(); }
		const TransformComponent& Transform() const { return GetComponent<TransformComponent>(); }

		SK_DEPRECATED("Use Entity::Name() instead")
			std::string& GetName() { return GetComponent<TagComponent>().Tag; }

		operator bool() { return IsValid(); }
		operator entt::entity() { return m_EntityHandle; }
		operator uint32_t() { return (uint32_t)m_EntityHandle; }
		bool operator==(const Entity& rhs) { return m_EntityHandle == rhs.m_EntityHandle && m_Scene == rhs.m_Scene; }
		bool operator!=(const Entity& rhs) { return !(*this == rhs); }

	private:
		entt::entity m_EntityHandle{ entt::null };
		Weak<Scene> m_Scene;

		friend class Scene;
		friend class Prefab;
	};

	template<typename TComponent, typename... TArgs>
	TComponent& Entity::AddComponent(TArgs&&... args)
	{
		SK_CORE_VERIFY(!HasComponent<TComponent>());
		return m_Scene.GetRef()->m_Registry.emplace<TComponent>(m_EntityHandle, std::forward<TArgs>(args)...);
	}

	template<typename TComponent, typename... TArgs>
	TComponent& Entity::AddOrReplaceComponent(TArgs&&... args)
	{
		return m_Scene.GetRef()->m_Registry.emplace_or_replace<TComponent>(m_EntityHandle, std::forward<TArgs>(args)...);
	}

	template<typename TComponent>
	void Entity::RemoveComponent()
	{
		SK_CORE_VERIFY(HasComponent<TComponent>());
		m_Scene.GetRef()->m_Registry.remove<TComponent>(m_EntityHandle);
	}

	template<typename TComponent>
	void Entity::RemoveComponentIsExists()
	{
		if (HasComponent<TComponent>())
			m_Scene.GetRef()->m_Registry.remove<TComponent>(m_EntityHandle);
	}

	template<typename TComponent>
	TComponent& Entity::GetComponent()
	{
		SK_CORE_VERIFY(HasComponent<TComponent>());
		return m_Scene.GetRef()->m_Registry.get<TComponent>(m_EntityHandle);
	}

	template<typename TComponent>
	const TComponent& Entity::GetComponent() const
	{
		SK_CORE_VERIFY(HasComponent<TComponent>());
		return m_Scene.GetRef()->m_Registry.get<TComponent>(m_EntityHandle);
	}

	template<typename TComponent>
	TComponent* Entity::TryGetComponent()
	{
		return m_Scene.GetRef()->m_Registry.try_get<TComponent>(m_EntityHandle);
	}

	template<typename TComponent>
	const TComponent* Entity::TryGetComponent() const
	{
		return m_Scene.GetRef()->m_Registry.try_get<TComponent>(m_EntityHandle);
	}

	template<typename TComponent>
	bool Entity::HasComponent() const
	{
		return m_Scene.GetRef()->m_Registry.all_of<TComponent>(m_EntityHandle);
	}

	template<typename... TComponents>
	bool Entity::HasAny() const
	{
		return m_Scene.GetRef()->m_Registry.any_of<TComponents...>(m_EntityHandle);
	}

	template<typename... TComponents>
	bool Entity::HasAll() const
	{
		return m_Scene.GetRef()->m_Registry.all_of<TComponents...>(m_EntityHandle);
	}

}