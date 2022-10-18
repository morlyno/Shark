#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Asset/Asset.h"

#include "Shark/Scene/Components.h"
#include "Shark/Scene/SceneCamera.h"

#include "Shark/Render/EditorCamera.h"

#include <entt.hpp>

namespace Shark {

	class SceneRenderer;
	class Entity;
	class Scene;

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
		void CopyTo(Ref<Scene> destScene);

		void IsEditorScene(bool isEditorScene) { m_IsEditorScene = isEditorScene; }
		bool IsEditorScene() { return m_IsEditorScene; }
		bool IsRunning() const { return m_IsRunning; }

		void OnScenePlay();
		void OnSceneStop();
		void OnSimulationPlay();

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts);
		void OnUpdateSimulate(TimeStep ts);

		void OnRenderRuntime(Ref<SceneRenderer> renderer);
		void OnRenderEditor(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera);
		void OnRenderSimulate(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera);

		void OnRender(Ref<SceneRenderer> renderer, const glm::mat4& viewProj);

		void OnFixedUpdate(float ts);

		Entity CloneEntity(Entity srcEntity);
		Entity CreateEntity(const std::string& tag = std::string{});
		Entity CreateEntityWithUUID(UUID uuid, const std::string& tag = std::string{});
		Entity CreateChildEntity(Entity parent, const std::string& tag = std::string{});
		Entity CreateChildEntityWithUUID(Entity parent, UUID uuid, const std::string& tag = std::string{});
		void DestroyEntity(Entity entity, bool destroyChildren = true);

		template<typename... TComponents, typename... TExclude>
		decltype(auto) GetAllEntitiesWith(entt::exclude_t<TExclude...> = {})
		{
			return m_Registry.view<TComponents...>(entt::exclude<TExclude...>);
		}

		Entity GetEntityByUUID(UUID uuid) const;
		Entity FindEntityByTag(const std::string& tag);
		Entity FindChildEntityByName(Entity entity, const std::string& name, bool recusive);

		bool IsValidEntity(Entity entity) const;
		bool ValidEntityID(UUID entityID) const;

		Entity GetActiveCameraEntity() const;
		UUID GetActiveCameraUUID() const { return m_ActiveCameraUUID; }
		void SetActiveCamera(UUID camera) { m_ActiveCameraUUID = camera; }
		void ResizeCameras(float width, float height);

		void SetViewportSize(uint32_t width, uint32_t height);
		uint32_t GetViewportWidth() const { return m_ViewportWidth; }
		uint32_t GetViewportHeight() const { return m_ViewportHeight; }

		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity) const;
		TransformComponent GetWorldSpaceTransform(Entity entity);

		bool ConvertToLocaSpace(Entity entity, glm::mat4& transformMatrix);
		bool ConvertToWorldSpace(Entity entity, glm::mat4& transformMatrix);
		bool ConvertToLocaSpace(Entity entity, TransformComponent& transform);
		bool ConvertToWorldSpace(Entity entity, TransformComponent& transform);
		bool ConvertToLocaSpace(Entity entity);
		bool ConvertToWorldSpace(Entity entity);

		const std::unordered_map<UUID, Entity>& GetEntityUUIDMap() const { return m_EntityUUIDMap; }

		template<typename TFunc>
		void Submit(const TFunc& func)
		{
			m_PostUpdateQueue.emplace_back(func);
		}

		static constexpr AssetType GetStaticType() { return AssetType::Scene; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		static Ref<Scene> Create() { return Ref<Scene>::Create(); }

	private:
		void DestroyEntityInternal(Entity entity, bool destroyChildren, bool first);
		void SetupBox2D();
		void OnPhyicsStep(TimeStep fixedTimeStep);

		void RenderEntity(const Ref<SceneRenderer>& renderer, Entity entity, const glm::mat4& parentTransform);

		void OnRigidBody2DComponentCreated(entt::registry& registry, entt::entity ent);
		void OnBoxCollider2DComponentCreated(entt::registry& registry, entt::entity ent);
		void OnCircleCollider2DComponentCreated(entt::registry& registry, entt::entity ent);

		void OnRigidBody2DComponentDestroyed(entt::registry& registry, entt::entity ent);
		void OnBoxCollider2DComponentDestroyed(entt::registry& registry, entt::entity ent);
		void OnCircleCollider2DComponentDestroyed(entt::registry& registry, entt::entity ent);
		void OnScriptComponentDestroyed(entt::registry& registry, entt::entity ent);
		void OnCameraComponentDestroyed(entt::registry& registry, entt::entity ent);

	private:
		entt::registry m_Registry;
		UUID m_ActiveCameraUUID = UUID::Null;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		std::unordered_map<UUID, Entity> m_EntityUUIDMap;

		bool m_IsEditorScene = false;
		bool m_IsRunning = false;

		std::vector<std::function<void()>> m_PostUpdateQueue;

		friend class Entity;
		friend class SceneHirachyPanel;
		friend class SceneSerializer;
	};

}