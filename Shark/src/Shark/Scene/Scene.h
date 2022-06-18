#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Asset/Asset.h"

#include "Shark/Scene/SceneCamera.h"
#include "Shark/Scene/Physics2DScene.h"
#include "Shark/Render/EditorCamera.h"

#include "Shark/Math/Bounds2.h"

#include <entt.hpp>

namespace Shark {

	class SceneRenderer;
	class Entity;
	class Scene;

	class ContactListener : public b2ContactListener
	{
	public:
		void BeginContact(b2Contact* contact) override;
		void EndContact(b2Contact* contact) override;

		void SetContext(const Ref<Scene>& context);

	private:
		Ref<Scene> m_Context;
	};

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

		void IsEditorScene(bool isEditorScene) { m_IsEditorScene = isEditorScene; }
		bool IsEditorScene() { return m_IsEditorScene; }

		void OnScenePlay();
		void OnSceneStop();
		void OnSimulateStart();

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts);
		void OnSimulate(TimeStep ts);

		void OnEventRuntime(Event& event);

		void OnRenderRuntimePreview(Ref<SceneRenderer> renderer, const glm::mat4& viewProj);
		void OnRenderRuntime(Ref<SceneRenderer> renderer);
		void OnRenderEditor(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera);
		void OnRenderSimulate(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera);

		Entity CloneEntity(Entity srcEntity);
		Entity CreateEntity(const std::string& tag = std::string{});
		Entity CreateEntityWithUUID(UUID uuid, const std::string& tag = std::string{});
		void DestroyEntity(Entity entity, bool destroyChildren = true);
		void DestroyAllEntities();

		template<typename Component>
		decltype(auto) GetAllEntitysWith()
		{
			return m_Registry.view<Component>();
		}

		Entity GetEntityByUUID(UUID uuid) const;

		bool IsValidEntity(Entity entity) const;

		Entity GetActiveCameraEntity() const;
		Entity GetRuntimeCamera();
		UUID GetActiveCameraUUID() const { return m_ActiveCameraUUID; }
		void SetActiveCamera(UUID camera) { m_ActiveCameraUUID = camera; }
		void ResizeCameras(float width, float height);

		void SetViewportSize(uint32_t width, uint32_t height) { m_ViewportWidth = width; m_ViewportHeight = height; ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight); }
		uint32_t GetViewportWidth() const { return m_ViewportWidth; }
		uint32_t GetViewportHeight() const { return m_ViewportHeight; }

		const std::unordered_map<UUID, Entity>& GetEntityUUIDMap() const { return m_EntityUUIDMap; }
		const Physics2DScene& GetPhysicsScene() const { return m_PhysicsScene; }

		static constexpr AssetType GetStaticType() { return AssetType::Scene; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		static Ref<Scene> Create() { return Ref<Scene>::Create(); }

	private:
		void DestroyEntityInternal(Entity entity, bool destroyChildren);
		void SetupBox2D();

		void OnRigidBody2DComponentCreated(entt::registry& registry, entt::entity entityID);
		void OnBoxCollider2DComponentCreated(entt::registry& registry, entt::entity entityID);
		void OnCircleCollider2DComponentCreated(entt::registry& registry, entt::entity entityID);
		
	private:
		entt::registry m_Registry;
		UUID m_ActiveCameraUUID = UUID::Null;
		entt::entity m_RuntimeCamera = entt::null;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		std::unordered_map<UUID, Entity> m_EntityUUIDMap;

		Physics2DScene m_PhysicsScene;
		ContactListener m_ContactListener;

		bool m_IsEditorScene = false;

		friend class Entity;
		friend class SceneHirachyPanel;
		friend class SceneSerializer;
	};

}