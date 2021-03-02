#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Render/EditorCamera.h"

#include <entt.hpp>

namespace Shark {

	class Entity;

	class Scean
	{
		friend class Entity;
		friend class SceanHirachyPanel;
	public:
		Scean();
		~Scean();

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts, EditorCamera& camera);

		Entity CreateEntity(const std::string& tag = std::string{});
		void DestroyEntity(Entity entity);

		template<typename... Components, typename Function>
		void ForEach(Function func)
		{
			if constexpr (sizeof...(Components) > 1)
			{
				auto group = m_Registry.group<Components...>();
				for (auto entityID : group)
				{
					Entity entity{ entityID, this };
					func(entity);
				}
			}
			else
			{
				auto view = m_Registry.view<Components...>();
				for (auto entityID : view)
				{
					Entity entity{ entityID, this };
					func(entity);
				}
			}
		}
	private:
		entt::registry m_Registry;
		entt::entity m_ActiveCameraID{ entt::null };
	};

}