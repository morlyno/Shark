#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Scene/SceneCamera.h"
#include "Shark/Render/EditorCamera.h"

#include "Shark/Asset/Asset.h"

#include "Shark/Scene/Physics2DScene.h"

#include <entt.hpp>

namespace Shark {

	class SceneRenderer;
	class Entity;

	class Scene : public Asset
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

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts);
		void OnSimulate(TimeStep ts);

		void OnRender(Ref<SceneRenderer> renderer, const glm::mat4& viewProj);

		void OnRenderRuntimePreview(Ref<SceneRenderer> renderer, const Camera& camera, const glm::mat4& view);
		void OnRenderRuntime(Ref<SceneRenderer> renderer);
		void OnRenderEditor(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera);
		void OnRenderSimulate(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera);

		Entity CloneEntity(Entity srcEntity);
		Entity CreateEntity(const std::string& tag = std::string{});
		Entity CreateEntityWithUUID(UUID uuid, const std::string& tag = std::string{});
		void DestroyEntity(Entity entity);

		template<typename Component>
		decltype(auto) GetAllEntitysWith()
		{
			return m_Registry.view<Component>();
		}

		Entity GetEntityByUUID(UUID uuid);

		SK_DEPRECATED("Use Entity::IsValid instead")
		bool IsValidEntity(Entity entity) const;

		Entity FindActiveCameraEntity();
		Entity GetRuntimeCamera();
		UUID GetActiveCameraUUID() const { return m_ActiveCameraUUID; }
		void SetActiveCamera(UUID camera) { m_ActiveCameraUUID = camera; }
		void ResizeCameras(float width, float height);

		void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight); }
		uint32_t GetViewportWidth() const { return m_ViewportWidth; }
		uint32_t GetViewportHeight() const { return m_ViewportHeight; }

		static constexpr AssetType GetStaticType() { return AssetType::Scene; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		void SetupBox2D();
	private:
		entt::registry m_Registry;
		UUID m_ActiveCameraUUID = UUID::Null();
		entt::entity m_RuntimeCamera = entt::null;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		Physics2DScene m_PhysicsScene;

		friend class Entity;
		friend class SceneHirachyPanel;
		friend class SceneSerializer;
	};

}