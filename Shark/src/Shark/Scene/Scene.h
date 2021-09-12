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

		Ref<Scene> GetCopy();
		void CopyInto(Ref<Scene> scene);

		void OnScenePlay();
		void OnSceneStop();

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts, EditorCamera& camera);
		
		void OnEventRuntime(Event& event);
		void OnEventEditor(Event& event);

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
			constexpr bool isRigidBodyComponent = std::is_same_v<Comp, RigidBodyComponent>;
			constexpr bool isBoxColliderComponent = std::is_same_v<Comp, BoxColliderComponent>;

			if constexpr (isRigidBodyComponent || isBoxColliderComponent)
			{
				// TODO: Make all components createable with a constructor
				Comp* srcComp = srcRegistry.try_get<Comp>(srcEntity);
				if (srcComp)
				{
					Comp& destComp = destRegistry.get_or_emplace<Comp>(destEntity);
					SK_CORE_ASSERT(&destRegistry == &m_Registry);
					OnComponentAdded<Comp>(Entity(destEntity, this), destComp);
					if constexpr (isRigidBodyComponent)
						destComp.Body.SetState(srcComp->Body.GetCurrentState());
					if constexpr (isBoxColliderComponent)
						destComp.Collider.SetState(srcComp->Collider.GetCurrentState());
				}
			}
			else
			{
				auto srcComp = srcRegistry.try_get<Comp>(srcEntity);
				if (srcComp)
					destRegistry.emplace_or_replace<Comp>(destEntity, *srcComp);
			}
		}

	private:
		template<typename Component>
		void OnComponentAdded(Entity entity, Component& comp);

	private:
		entt::registry m_Registry;
		entt::entity m_ActiveCameraID{ entt::null };
		World m_World;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		std::filesystem::path m_FilePath;

		friend class Entity;
		friend class SceneHirachyPanel;
		friend class SceneSerializer;
	};

}