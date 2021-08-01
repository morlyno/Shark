#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/Physiks/World.h"

#include "Shark/Event/Event.h"
#include "Shark/Event/ApplicationEvent.h"

#include <entt.hpp>

namespace Shark {

	class Entity;

	class Scene : public RefCount
	{
	public:
		Scene();
		~Scene();

		Scene(const Scene& other) = delete;
		Scene& operator=(const Scene& other) = delete;

		Scene(Scene&& other);
		Scene& operator=(Scene&& other);

		Ref<Scene> Copy();
		void Copy(Ref<Scene> scene);

		void OnScenePlay();
		void OnSceneStop();

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

		void SetFilePath(const std::string& filepath) { m_FilePath = filepath; }
		const std::string& GetFilePath() const { return m_FilePath; }

		void OnEvent(Event& event);

	private:
		bool OnSelectionChanged(SelectionChangedEvent& event);

		template<typename Component>
		void OnComponentAdded(Entity entity, Component& comp);
	private:
		entt::registry m_Registry;
		entt::entity m_ActiveCameraID{ entt::null };
		entt::entity m_SelectedEntity{ entt::null };
		World m_World;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		std::string m_FilePath;

		friend class Entity;
		friend class SceneHirachyPanel;
		friend class SceneSerializer;
	};

}