#pragma once

#include "Shark/Scean/Scean.h"
#include <entt.hpp>

namespace Shark {

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity entityhandle, const WeakRef<Scean>& scean);
		Entity(const Entity&) = default;

		template<typename Component, typename... Args>
		Component& AddComponent(Args&&... args)
		{
			auto& comp = m_Scean->m_Registry.emplace<Component>(m_EntityHandle, std::forward<Args>(args)...);
			m_Scean->OnComponentAdded(*this, comp);
			return comp;
		}

		template<typename Component>
		void RemoveComponent()
		{
			m_Scean->m_Registry.remove<Component>(m_EntityHandle);
		}

		template<typename... Components>
		decltype(auto) GetComponent()
		{
			return m_Scean->m_Registry.get<Components...>(m_EntityHandle);
		}

		template<typename Component>
		bool HasComponent()
		{
			return m_Scean->m_Registry.has<Component>(m_EntityHandle);
		}

		operator entt::entity() { return m_EntityHandle; }
		operator bool() { return m_EntityHandle != entt::null; }
		operator uint32_t() { return (uint32_t)m_EntityHandle; }
		bool operator==(const Entity& rhs) { return m_EntityHandle == rhs.m_EntityHandle && m_Scean == rhs.m_Scean; }
	private:
		entt::entity m_EntityHandle{ entt::null };
		WeakRef<Scean> m_Scean;
	};

}