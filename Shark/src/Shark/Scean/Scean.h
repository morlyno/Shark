#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/Scean/RigidBody.h"

#include <entt.hpp>

namespace Shark {

	class Entity;

	class Scean
	{
		friend class Entity;
		friend class SceanHirachyPanel;
		friend class SceanSerializer;
	public:
		Scean();
		~Scean();

		void OnSceanPlay();
		void OnSceanStop();

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts, EditorCamera& camera);

		Entity CreateEntity(const std::string& tag = std::string{});
		void DestroyEntity(Entity entity);

		template<typename Component>
		void OnComponentAdded(Entity entity, Component& comp);

		Entity GetActiveCamera();
		void ResizeCameras(float width, float height);

		void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight); }

		template<typename... Components, typename Function>
		void ForEach(const Function& func)
		{
			if constexpr (sizeof...(Components) == 0)
			{

				// doesnt work with Shark::Entity
				m_Registry.each(func);
			}
			else if constexpr (sizeof...(Components) == 1)
			{
				auto view = m_Registry.view<Components...>();
				for (auto entityID : view)
				{
					Entity entity{ entityID, this };
					func(entity);
				}
			}
			else
			{
				auto group = m_Registry.group<Components...>();
				for (auto entityID : group)
				{
					Entity entity{ entityID, this };
					func(entity);
				}
			}
		}

		World& GetWorld() { return m_World; }
	private:
		entt::registry m_Registry;
		entt::entity m_ActiveCameraID{ entt::null };
		World m_World;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		struct SceanState
		{
			entt::registry Registry;
			entt::entity ActiveCameraID;
			std::unordered_map<uint32_t, RigidBodySpecs> RigidBodyStates;
		};
		SceanState m_SceanState;
	};

}