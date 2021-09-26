#pragma once

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Components/TagComponent.h"
#include "Shark/Scene/Components/TransformComponent.h"
#include "Shark/Scene/Components/IDComponent.h"

#include "Shark/Debug/Instrumentor.h"
#include <entt.hpp>

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

		template<typename Component, typename... Args>
		Component& AddComponent(Args&&... args)
		{
			SK_PROFILE_FUNCTION();

			SK_CORE_ASSERT(!HasComponent<Component>());
			return m_Scene->m_Registry.emplace<Component>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename Component, typename... Args>
		Component& AddOrReplaceComponent(Args&&... args)
		{
			SK_PROFILE_FUNCTION();

			return m_Scene->m_Registry.emplace_or_replace<Component>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename Component>
		void RemoveComponent()
		{
			SK_PROFILE_FUNCTION();

			SK_CORE_ASSERT(HasComponent<Component>());
			m_Scene->m_Registry.remove<Component>(m_EntityHandle);
		}

		template<typename Component>
		Component& GetComponent()
		{
			SK_PROFILE_FUNCTION();

			SK_CORE_ASSERT(HasComponent<Component>());
			return m_Scene->m_Registry.get<Component>(m_EntityHandle);
		}

		template<typename Component>
		Component* TryGetComponent()
		{
			SK_PROFILE_FUNCTION();

			return m_Scene->m_Registry.try_get<Component>(m_EntityHandle);
		}

		UUID GetUUID();
		const std::string& GetName();
		TransformComponent& GetTransform();

		template<typename Component>
		bool HasComponent() const
		{
			SK_PROFILE_FUNCTION();
			
			return m_Scene->m_Registry.all_of<Component>(m_EntityHandle);
		}

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