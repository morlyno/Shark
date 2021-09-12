#pragma once

#include "Shark/Scene/Scene.h"
#include <entt.hpp>

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	class Entity
	{
	public:
		Entity() = default;
		Entity(uint32_t entityhandle, const Weak<Scene>& scene);
		Entity(entt::entity entityhandle, const Weak<Scene>& scene);
		Entity(const Entity&) = default;

		Weak<Scene> GetScene() const { return m_Scene; }
		entt::entity GetHandle() const { return m_EntityHandle; }

		template<typename Component>
		Component& AddComponent()
		{
			SK_PROFILE_FUNCTION();

			SK_CORE_ASSERT(!HasComponent<Component>());
			auto& comp = m_Scene->m_Registry.emplace<Component>(m_EntityHandle);
			m_Scene->OnComponentAdded(*this, comp);
			return comp;
		}

		// Function will be removed in the future. DO NOT USE!
		template<typename Component>
		Component& AddComponent(const Component& c)
		{
			SK_CORE_TRACE("Function will be removed in the future. DO NOT USE!");

			SK_CORE_ASSERT(!HasComponent<Component>());
			auto& comp = m_Scene->m_Registry.emplace<Component>(m_EntityHandle, c);
			m_Scene->OnComponentAdded(*this, comp);
			return comp;
		}

		template<typename Component>
		void RemoveComponent()
		{
			SK_PROFILE_FUNCTION();

			SK_CORE_ASSERT(HasComponent<Component>());
			m_Scene->m_Registry.remove<Component>(m_EntityHandle);
		}

		template<typename... Components>
		decltype(auto) GetComponent()
		{
			SK_PROFILE_FUNCTION();

			SK_CORE_ASSERT(HasComponent<Components...>());
			return m_Scene->m_Registry.get<Components...>(m_EntityHandle);
		}

		template<typename Component>
		Component& TryAddComponent()
		{
			SK_PROFILE_FUNCTION();

			if (!HasComponent<Component>())
				return AddComponent<Component>();
			return GetComponent<Component>();
		}

		template<typename Component>
		bool HasComponent() const { SK_PROFILE_FUNCTION(); return m_Scene->m_Registry.has<Component>(m_EntityHandle); }

		bool IsValid() const { SK_PROFILE_FUNCTION(); return m_Scene->m_Registry.valid(m_EntityHandle); }
		bool IsNull() const { return m_EntityHandle == entt::null; }

		operator entt::entity() { return m_EntityHandle; }
		operator bool() { return !IsNull() && IsValid(); }
		operator uint32_t() { return (uint32_t)m_EntityHandle; }
		bool operator==(const Entity& rhs) { return m_EntityHandle == rhs.m_EntityHandle && m_Scene == rhs.m_Scene; }
	private:
		entt::entity m_EntityHandle{ entt::null };
		Weak<Scene> m_Scene;

		friend Scene;
	};

}