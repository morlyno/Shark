#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Render/EditorCamera.h"

#include <entt.hpp>

class b2World;

namespace Shark {

	class Entity;

	class Scene : public RefCount
	{
	public:
		Scene();
		~Scene();
		
		Scene(const Scene& other) = delete;
		Scene& operator=(const Scene& other) = delete;
		Scene(Scene&& other) = default;
		Scene& operator=(Scene&& other) = default;

		static Ref<Scene> Copy(Ref<Scene> srcScene);

		void OnScenePlay();
		void OnSceneStop();

		void OnSimulateStart();
		void OnSimulateStop();

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts, EditorCamera& camera);
		void OnSimulate(TimeStep ts, bool subStep = false);

		void Render(EditorCamera& camera);
		
		Entity CloneEntity(Entity srcEntity);
		Entity CreateEntity(const std::string& tag = std::string{});
		Entity CreateEntityWithUUID(UUID uuid, const std::string& tag = std::string{});
		void DestroyEntity(Entity entity);

		Entity GetEntityByUUID(UUID uuid);

		bool IsValidEntity(Entity entity) const;

		Entity GetActiveCamera();
		UUID GetActiveCameraUUID() const { return m_ActiveCameraUUID; }
		void SetActiveCamera(UUID camera) { m_ActiveCameraUUID = camera; }
		void ResizeCameras(float width, float height);

		void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight); }

		void SetFilePath(const std::filesystem::path& filepath) { m_FilePath = filepath; }
		const std::filesystem::path& GetFilePath() const { return m_FilePath; }

	private:
		void SetupBox2D();
	private:
		entt::registry m_Registry;
		UUID m_ActiveCameraUUID = 0;
		entt::entity m_RuntimeCamera = entt::null;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		b2World* m_PhysicsWorld2D = nullptr;

		std::filesystem::path m_FilePath;

		friend class Entity;
		friend class SceneHirachyPanel;
		friend class SceneSerializer;
	};

}