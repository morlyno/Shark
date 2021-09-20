#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/TimeStep.h"
#include "Shark/Render/EditorCamera.h"

#include "Shark/Event/Event.h"
#include "Shark/Event/ApplicationEvent.h"

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

		Ref<Scene> GetCopy();
		void CopyInto(Ref<Scene> scene);

		void OnScenePlay();
		void OnSceneStop();

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts, EditorCamera& camera);
		
		Entity CopyEntity(Entity srcEntity);
		Entity CreateEntity(const std::string& tag = std::string{});
		Entity CreateEntity(entt::entity hint, const std::string& tag = std::string{});
		void DestroyEntity(Entity entity);

		bool IsValidEntity(Entity entity) const;
		uint32_t AliveEntitys() const { return (uint32_t)m_Registry.alive(); }

		Entity GetActiveCamera();
		uint32_t GetActiveCameraID() const { return (uint32_t)m_ActiveCameraID; }
		void SetActiveCamera(Entity cameraentity);
		void ResizeCameras(float width, float height);

		void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight); }

		void SetFilePath(const std::filesystem::path& filepath) { m_FilePath = filepath; }
		const std::filesystem::path& GetFilePath() const { return m_FilePath; }

	private:
		template<typename Comp>
		void CopyComponentIfExists(entt::entity srcEntity, entt::registry& srcRegistry, entt::entity destEntity, entt::registry& destRegistry)
		{
			auto srcComp = srcRegistry.try_get<Comp>(srcEntity);
			if (srcComp)
				destRegistry.emplace_or_replace<Comp>(destEntity, *srcComp);
		}

	private:
		entt::registry m_Registry;
		entt::entity m_ActiveCameraID{ entt::null };

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		b2World* m_PhysicsWorld2D = nullptr;

		std::filesystem::path m_FilePath;

		friend class Entity;
		friend class SceneHirachyPanel;
		friend class SceneSerializer;
	};

}