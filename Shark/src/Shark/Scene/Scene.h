#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/UUID.h"
#include "Shark/Core/TimeStep.h"

#include "Shark/Asset/Asset.h"

#include "Shark/Scene/Components.h"
#include "Shark/Scene/SceneCamera.h"
#include "Shark/Scene/Physics2DScene.h"
#include "Shark/Render/EditorCamera.h"
#include "Shark/Render/Environment.h"

#include <entt.hpp>

namespace Shark {

	class SceneRenderer;
	struct SceneRendererCamera;
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

	struct PointLight
	{
		glm::vec3 Position;
		float Intensity;
		glm::vec3 Radiance;
		float Radius;
		float Falloff;
		float P0, P1, P2;
	};

	struct DirectionalLight
	{
		glm::vec4 Radiance;
		glm::vec3 Direction;
		float Intensity;
	};

	struct LightEnvironment
	{
		static constexpr uint32_t MaxDirectionLights = 5;

		uint32_t DirectionalLightCount = 0;
		DirectionalLight DirectionalLights[MaxDirectionLights]{};
		std::vector<PointLight> PointLights;

		Ref<Environment> SceneEnvironment;
		float EnvironmentIntensity = 1.0f;
		float SkyboxLod = 0.0f;
	};

	class Scene : public Asset
	{
	public:
		Scene(const std::string& name = "Untitled");
		~Scene();
		
		Scene(const Scene& other) = delete;
		Scene& operator=(const Scene& other) = delete;
		Scene(Scene&& other) = default;
		Scene& operator=(Scene&& other) = default;

		static Ref<Scene> Copy(Ref<Scene> srcScene);
		void CopyTo(Ref<Scene> destScene);

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		void DestroyEntities();

		void IsEditorScene(bool isEditorScene) { m_IsEditorScene = isEditorScene; }
		bool IsEditorScene() { return m_IsEditorScene; }
		bool IsRunning() const { return m_IsRunning; }

		void OnScenePlay();
		void OnSceneStop();
		void OnSimulationPlay();
		void OnSimulationStop();

		void OnUpdateRuntime(TimeStep ts);
		void OnUpdateEditor(TimeStep ts);
		void OnUpdateSimulate(TimeStep ts);

		void OnRenderRuntime(Ref<SceneRenderer> renderer);
		void OnRenderEditor(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera);
		void OnRenderSimulate(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera);

		void OnRender(Ref<SceneRenderer> renderer, const SceneRendererCamera& camera);

		Ref<Environment> GetEnvironment() const { return m_LightEnvironment.SceneEnvironment; }
		float GetEnvironmentIntesity() const { return m_LightEnvironment.EnvironmentIntensity; }
		float GetSkyboxLod() const { return m_LightEnvironment.SkyboxLod; }

		const LightEnvironment& GetLightEnvironment() const { return m_LightEnvironment; }
		const std::vector<PointLight>& GetPointLights() const { return m_LightEnvironment.PointLights; }

		Entity CloneEntity(Entity srcEntity);
		Entity CreateEntity(const std::string& tag = std::string{});
		Entity CreateEntityWithUUID(UUID uuid, const std::string& tag = std::string{});
		Entity CreateChildEntity(Entity parent, const std::string& tag = std::string{});
		Entity CreateChildEntityWithUUID(Entity parent, UUID uuid, const std::string& tag = std::string{});
		void DestroyEntity(Entity entity, bool destroyChildren = true);

		template<typename Component>
		decltype(auto) GetAllEntitysWith()
		{
			return m_Registry.view<Component>();
		}

#if 0
		Entity InstantiateMesh(Ref<Mesh> mesh);
		void InstantiateSubMesh(Ref<Mesh> mesh, const MeshNode& node, Entity parent);
#endif

		Entity InstantiateMesh(Ref<Mesh> mesh);
		void InstantiateSubmesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, const MeshNode& node, Entity parent);

		Entity TryGetEntityByUUID(UUID uuid) const;
		Entity FindEntityByTag(const std::string& tag);
		Entity FindChildEntityByName(Entity entity, const std::string& name, bool recusive);

		bool IsValidEntity(Entity entity) const;
		bool ValidEntityID(UUID entityID) const;

		Entity GetActiveCameraEntity() const;
		UUID GetActiveCameraUUID() const { return m_ActiveCameraUUID; }
		void SetActiveCamera(UUID camera) { m_ActiveCameraUUID = camera; }
		void ResizeCameras() { ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight); }
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

		// returns all entities sorted by UUID
		std::vector<Entity> GetEntitiesSorted();
		const Physics2DScene& GetPhysicsScene() const { return m_PhysicsScene; }

		template<typename TFunc>
		void Submit(const TFunc& func)
		{
			m_PostUpdateQueue.emplace_back(func);
		}

		void Step(uint32_t frames) { m_StepFrames = frames; }
		void SetPaused(bool paused) { m_Paused = paused; }
		bool IsPaused() const { return m_Paused; }

	public:
		static constexpr AssetType GetStaticType() { return AssetType::Scene; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		void DestroyEntityInternal(Entity entity, bool destroyChildren, bool first);

		void OnPhysics2DPlay(bool connectWithScriptingAPI);
		void OnPhysics2DStop();
		void OnPhyicsStep(TimeStep fixedTimeStep);

		void OnRigidBody2DComponentCreated(entt::registry& registry, entt::entity ent);
		void OnBoxCollider2DComponentCreated(entt::registry& registry, entt::entity ent);
		void OnCircleCollider2DComponentCreated(entt::registry& registry, entt::entity ent);
		void OnCameraComponentCreated(entt::registry& registry, entt::entity ent);

		void OnRigidBody2DComponentDestroyed(entt::registry& registry, entt::entity ent);
		void OnBoxCollider2DComponentDestroyed(entt::registry& registry, entt::entity ent);
		void OnCircleCollider2DComponentDestroyed(entt::registry& registry, entt::entity ent);
		void OnScriptComponentDestroyed(entt::registry& registry, entt::entity ent);

	private:
		std::string m_Name;

		entt::registry m_Registry;
		UUID m_ActiveCameraUUID = UUID::Null;

		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		std::unordered_map<UUID, Entity> m_EntityUUIDMap;

		Physics2DScene m_PhysicsScene;
		ContactListener m_ContactListener;

		bool m_IsEditorScene = false;
		bool m_IsRunning = false;

		bool m_Paused = false;
		uint32_t m_StepFrames = 0;

		LightEnvironment m_LightEnvironment;

		std::vector<std::function<void()>> m_PostUpdateQueue;

		friend class Entity;
		friend class SceneHirachyPanel;
		friend class SceneSerializer;
		friend class ECSDebugPanel;
	};

}