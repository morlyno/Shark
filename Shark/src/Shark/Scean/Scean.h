#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/Physiks/World.h"

#include <entt.hpp>

namespace Shark {

	class Entity;

	class Scean : public RefCount
	{
		friend class Entity;
		friend class SceanHirachyPanel;
		friend class SceanSerializer;

		static Scean Copy(const Scean& src);
	public:
		Scean();
		~Scean();

		Scean(const Scean& other);
		Scean& operator=(const Scean& other);

		Scean(Scean&& other);
		Scean& operator=(Scean&& other);

		void OnSceanPlay();
		void OnSceanStop();

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts, EditorCamera& camera);

		Entity CreateEntity(Entity other, bool hint = false);
		Entity CreateEntity(const std::string& tag = std::string{});
		void DestroyEntity(Entity entity);

		template<typename Component>
		void OnComponentAdded(Entity entity, Component& comp);

		Entity GetActiveCamera();
		void ResizeCameras(float width, float height);

		void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight); }

		World& GetWorld() { return m_World; }

		static Ref<Scean> Create();
	private:
		entt::registry m_Registry;
		entt::entity m_ActiveCameraID{ entt::null };
		World m_World;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	};

}