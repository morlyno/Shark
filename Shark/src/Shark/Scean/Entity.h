#pragma once

#include "Shark/Scean/Scean.h"
#include <entt.hpp>

namespace Shark {

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity entityhandle, const Weak<Scean>& scean);
		Entity(const Entity&) = default;

		Weak<Scean> GetScean() const { return m_Scean; }

		template<typename Component>
		Component& AddComponent()
		{
			SK_CORE_ASSERT(!HasComponent<Component>());
			auto& comp = m_Scean->m_Registry.emplace<Component>(m_EntityHandle);
			m_Scean->OnComponentAdded(*this, comp);
			return comp;
		}

		template<typename Component>
		Component& AddComponent(const Component& c)
		{
			SK_CORE_ASSERT(!HasComponent<Component>());
			auto& comp = m_Scean->m_Registry.emplace<Component>(m_EntityHandle, c);
			m_Scean->OnComponentAdded(*this, comp);
			return comp;
		}

		template<typename Component>
		void RemoveComponent()
		{
			SK_CORE_ASSERT(HasComponent<Component>());
			m_Scean->m_Registry.remove<Component>(m_EntityHandle);
		}

		template<typename... Components>
		decltype(auto) GetComponent()
		{
			SK_CORE_ASSERT(HasComponent<Components...>());
			return m_Scean->m_Registry.get<Components...>(m_EntityHandle);
		}

		template<typename Component>
		bool HasComponent() const { return m_Scean->m_Registry.has<Component>(m_EntityHandle); }

		bool IsValid() const { return m_Scean->m_Registry.valid(m_EntityHandle); }

		operator entt::entity() { return m_EntityHandle; }
		operator bool() { return m_EntityHandle != entt::null; }
		operator uint32_t() { return (uint32_t)m_EntityHandle; }
		bool operator==(const Entity& rhs) { return m_EntityHandle == rhs.m_EntityHandle && m_Scean == rhs.m_Scean; }
	private:
		entt::entity m_EntityHandle{ entt::null };
		Weak<Scean> m_Scean;
	};

}