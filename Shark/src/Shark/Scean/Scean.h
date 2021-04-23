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

		void OnEventRuntime(Event& event);
		void OnEventEditor(Event& event);

		Entity CreateEntity(Entity other, bool hint = false);
		Entity CreateEntity(const std::string& tag = std::string{});
		void DestroyEntity(Entity entity);

		Entity GetActiveCamera();
		void SetActiveCamera(Entity cameraentity);
		void ResizeCameras(float width, float height);

		bool IsValidEntity(Entity entity);

		void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight); }

		void AddEditorData(bool editordata) { m_AddEditorData = editordata; }
		bool AddEditorDataEnabled() const { return m_AddEditorData; }

		static Ref<Scean> Create();
	private:
		template<typename Component>
		void OnComponentAdded(Entity entity, Component& comp);
	private:
		entt::registry m_Registry;
		entt::entity m_ActiveCameraID{ entt::null };
		World m_World;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_AddEditorData = false;

		friend class Entity;
		friend class SceanHirachyPanel;
		friend class SceanSerializer;
	};

}