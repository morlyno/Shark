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
	private:
		entt::registry m_Registry;
	};

}