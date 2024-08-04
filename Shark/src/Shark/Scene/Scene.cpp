#include "skpch.h"
#include "Scene.h"

#include "Shark/Asset/AssetManager.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/SceneRenderer.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/ScriptGlue.h"

#include "Shark/Math/Math.h"

#include "Shark/Debug/enttDebug.h"
#include "Shark/Debug/Profiler.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_fixture.h>
#include "box2d/b2_contact.h"
#include "box2d/b2_friction_joint.h"
#include "box2d/b2_distance_joint.h"
#include "box2d/b2_revolute_joint.h"
#include "box2d/b2_prismatic_joint.h"
#include "box2d/b2_pulley_joint.h"

namespace Shark {

	template<typename Component>
	void CopyComponents(entt::registry& srcRegistry, entt::registry& destRegistry, const std::unordered_map<UUID, Entity>& entityUUIDMap)
	{
		SK_PROFILE_FUNCTION();

		auto view = srcRegistry.view<Component>();
		for (auto srcEntity : view)
		{
			Entity destEntity = entityUUIDMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);

			auto& comp = srcRegistry.get<Component>(srcEntity);
			destRegistry.emplace_or_replace<Component>(destEntity, comp);
		}
	}

	template<typename Component>
	void CopyComponentIfExists(entt::entity srcEntity, entt::registry& srcRegistry, entt::entity destEntity, entt::registry& destRegistry)
	{
		SK_PROFILE_FUNCTION();

		if (auto* comp = srcRegistry.try_get<Component>(srcEntity))
			destRegistry.emplace_or_replace<Component>(destEntity, *comp);
	}

	static b2BodyType SharkBodyTypeToBox2D(RigidBody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
			case RigidBody2DComponent::BodyType::None:        return b2_staticBody; // use static body as default fallback
			case RigidBody2DComponent::BodyType::Static:      return b2_staticBody;
			case RigidBody2DComponent::BodyType::Dynamic:     return b2_dynamicBody;
			case RigidBody2DComponent::BodyType::Kinematic:   return b2_kinematicBody;
		}

		SK_CORE_ASSERT(false, "Unkown Body Type");
		return b2_staticBody;
	}


	Scene::Scene(const std::string& name)
		: m_Name(name)
	{
		m_Registry.on_construct<CameraComponent>().connect<&Scene::OnCameraComponentCreated>(this);
	}

	Scene::~Scene()
	{
		m_Registry.on_construct<CameraComponent>().disconnect<&Scene::OnCameraComponentCreated>(this);
	}

	Ref<Scene> Scene::Copy(Ref<Scene> srcScene)
	{
		Ref<Scene> newScene = Ref<Scene>::Create();
		srcScene->CopyTo(newScene);
		return newScene;
	}

	void Scene::CopyTo(Ref<Scene> destScene)
	{
		SK_PROFILE_FUNCTION();

		destScene->m_Name = m_Name;
		destScene->m_ViewportWidth = m_ViewportWidth;
		destScene->m_ViewportHeight = m_ViewportHeight;
		destScene->m_ActiveCameraUUID = m_ActiveCameraUUID;

		destScene->m_Registry.clear();
		destScene->m_EntityUUIDMap.clear();

		auto& destRegistry = destScene->m_Registry;
		auto view = m_Registry.view<IDComponent>();
		for (auto e : view)
		{
			Entity srcEntity{ e, this };

			UUID uuid = srcEntity.GetUUID();
			destScene->m_EntityUUIDMap[uuid] = destScene->CreateEntityWithUUID(uuid, srcEntity.GetName());
		}

		ForEach(AllComponents, [&]<typename TComp>()
		{
			CopyComponents<TComp>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		});

	}

	void Scene::DestroyEntities()
	{
		m_Registry.clear();
		m_ActiveCameraUUID = UUID::Invalid;
		m_EntityUUIDMap.clear();
	}

	void Scene::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();
		
		m_IsRunning = true;

		ScriptEngine::InitializeRuntime(this);

		OnPhysics2DPlay(true);

		m_Registry.on_construct<RigidBody2DComponent>().connect<&Scene::OnRigidBody2DComponentCreated>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().connect<&Scene::OnBoxCollider2DComponentCreated>(this);
		m_Registry.on_construct<CircleCollider2DComponent>().connect<&Scene::OnCircleCollider2DComponentCreated>(this);

		m_Registry.on_destroy<RigidBody2DComponent>().connect<&Scene::OnRigidBody2DComponentDestroyed>(this);
		m_Registry.on_destroy<BoxCollider2DComponent>().connect<&Scene::OnBoxCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<CircleCollider2DComponent>().connect<&Scene::OnCircleCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<ScriptComponent>().connect<&Scene::OnScriptComponentDestroyed>(this);

		// Create Scripts
		{
			SK_PROFILE_SCOPED("Instantiate Scripts");

			{
				auto view = m_Registry.view<ScriptComponent>();
				for (auto entityID : view)
				{
					Entity entity{ entityID, this };
					ScriptEngine::InstantiateEntity(entity, false, false);
				}

				for (auto entityID : view)
				{
					Entity entity{ entityID, this };
					ScriptEngine::InitializeFields(entity);
				}
			}

			{
				SK_PROFILE_SCOPED("Invoke OnCreate");

				for (const auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
					MethodThunks::OnCreate(gcHandle);
			}
		}

		// Setup Cameras
		bool activeCameraFound = false;
		if (m_ActiveCameraUUID != UUID::Invalid)
		{
			Entity activeCamera = TryGetEntityByUUID(m_ActiveCameraUUID);
			if (activeCamera.AllOf<CameraComponent>())
				activeCameraFound = true;
		}

		if (!activeCameraFound)
		{
			Entity cameraEntity = CreateEntity("Fallback Camera");
			cameraEntity.Transform().Translation.z = -10.0f;
			cameraEntity.AddComponent<CameraComponent>(true);

			m_ActiveCameraUUID = cameraEntity.GetUUID();
		}
		ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	void Scene::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();

		m_IsRunning = false;

		// Destroy Scripts
		{
			SK_PROFILE_SCOPED("Scene::OnScenePlay::DestroyScripts");

			for (auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
				MethodThunks::OnDestroy(gcHandle);

			ScriptEngine::ShutdownRuntime();
		}

		m_Registry.on_construct<RigidBody2DComponent>().disconnect<&Scene::OnRigidBody2DComponentCreated>(this);
		m_Registry.on_construct<BoxCollider2DComponent>().disconnect<&Scene::OnBoxCollider2DComponentCreated>(this);
		m_Registry.on_construct<CircleCollider2DComponent>().disconnect<&Scene::OnCircleCollider2DComponentCreated>(this);

		m_Registry.on_destroy<RigidBody2DComponent>().disconnect<&Scene::OnRigidBody2DComponentDestroyed>(this);
		m_Registry.on_destroy<BoxCollider2DComponent>().disconnect<&Scene::OnBoxCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<CircleCollider2DComponent>().disconnect<&Scene::OnCircleCollider2DComponentDestroyed>(this);
		m_Registry.on_destroy<ScriptComponent>().disconnect<&Scene::OnScriptComponentDestroyed>(this);
	}

	void Scene::OnSimulationPlay()
	{
		SK_PROFILE_FUNCTION();
		
		OnPhysics2DPlay(false);
	}

	void Scene::OnSimulationStop()
	{
		SK_PROFILE_FUNCTION();

		OnPhysics2DStop();
	}

	void Scene::OnUpdateRuntime(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnUpdateRuntime");

		if (m_Paused)
		{
			if (m_StepFrames == 0)
				return;
			m_StepFrames--;
		}

		for (auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
			MethodThunks::OnUpdate(gcHandle, ts);

		{
			m_PhysicsScene.Step(ts);

			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto e : view)
			{
				Entity entity{ e, this };
				const auto& rb2d = view.get<RigidBody2DComponent>(entity);
				auto& transform = entity.Transform();
				if (entity.HasParent())
				{
					glm::mat4 localTransform = glm::inverse(GetWorldSpaceTransformMatrix(entity.Parent())) * Phyiscs2DUtils::GetMatrix(rb2d.RuntimeBody);
					TransformComponent localBodyTransform;
					Math::DecomposeTransform(localTransform, localBodyTransform.Translation, localBodyTransform.Rotation, localBodyTransform.Scale);
					transform.Translation.x = localBodyTransform.Translation.x;
					transform.Translation.y = localBodyTransform.Translation.y;
					transform.Rotation.z = localBodyTransform.Rotation.z;
					continue;
				}

				const auto& pos = rb2d.RuntimeBody->GetPosition();
				transform.Translation.xy = Phyiscs2DUtils::FromBody(rb2d.RuntimeBody);
				transform.Rotation.z = rb2d.RuntimeBody->GetAngle();
			}
		}

		for (const auto& fn : m_PostUpdateQueue)
			fn();
		m_PostUpdateQueue.clear();
	}

	void Scene::OnUpdateEditor(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();
	}

	void Scene::OnUpdateSimulate(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnUpdateSimulate");

		if (m_Paused)
		{
			if (m_StepFrames == 0)
				return;
			m_StepFrames--;
		}

		m_PhysicsScene.Step(ts);

		auto view = m_Registry.view<RigidBody2DComponent>();
		for (auto e : view)
		{
			Entity entity{ e, this };
			const auto& rb2d = view.get<RigidBody2DComponent>(entity);
			auto& transform = entity.Transform();
			if (entity.HasParent())
			{
				glm::mat4 localTransform = glm::inverse(GetWorldSpaceTransformMatrix(entity.Parent())) * Phyiscs2DUtils::GetMatrix(rb2d.RuntimeBody);
				TransformComponent localBodyTransform;
				Math::DecomposeTransform(localTransform, localBodyTransform.Translation, localBodyTransform.Rotation, localBodyTransform.Scale);
				transform.Translation.x = localBodyTransform.Translation.x;
				transform.Translation.y = localBodyTransform.Translation.y;
				transform.Rotation.z = localBodyTransform.Rotation.z;
				continue;
			}

			const auto& pos = rb2d.RuntimeBody->GetPosition();
			transform.Translation.xy = Phyiscs2DUtils::FromBody(rb2d.RuntimeBody);
			transform.Rotation.z = rb2d.RuntimeBody->GetAngle();
		}
	}

	void Scene::OnRenderRuntime(Ref<SceneRenderer> renderer)
	{
		Entity cameraEntity = TryGetEntityByUUID(m_ActiveCameraUUID);
		TransformComponent transform = GetWorldSpaceTransform(cameraEntity);

		SceneRendererCamera camera;
		camera.Position = transform.Translation;
		camera.View = glm::inverse(transform.CalcTransform());

		const auto& component = cameraEntity.GetComponent<CameraComponent>();
		camera.Projection = component.GetProjection();

		OnRender(renderer, camera);
	}

	void Scene::OnRenderEditor(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera)
	{
		OnRender(renderer, { editorCamera.GetView(), editorCamera.GetProjection(), editorCamera.GetPosition() });
	}

	void Scene::OnRenderSimulate(Ref<SceneRenderer> renderer, const EditorCamera& editorCamera)
	{
		OnRender(renderer, { editorCamera.GetView(), editorCamera.GetProjection(), editorCamera.GetPosition() });
	}

	void Scene::OnRender(Ref<SceneRenderer> renderer, const SceneRendererCamera& camera)
	{
		SK_PROFILE_FUNCTION();
		SK_PERF_SCOPED("Scene::OnRender");

		m_LightEnvironment = LightEnvironment();

		{
			auto view = m_Registry.view<SkyComponent>();
			if (view.size())
			{
				Entity entity{ view.front(), this };
				SkyComponent skyComp = entity.GetComponent<SkyComponent>();
				if (skyComp.DynamicSky && !AssetManager::IsValidAssetHandle(skyComp.SceneEnvironment))
				{
					// TODO(moro): Dynamic sky (Preetham sky model)
					skyComp.SceneEnvironment = AssetManager::CreateMemoryOnlyAsset<Environment>(Renderer::GetBlackTextureCube(), Renderer::GetBlackTextureCube());
				}

				m_LightEnvironment.SceneEnvironment = AssetManager::GetAssetAsync<Environment>(skyComp.SceneEnvironment);
				m_LightEnvironment.EnvironmentIntensity = skyComp.Intensity;
				m_LightEnvironment.SkyboxLod = skyComp.Lod;
			}

			if (!m_LightEnvironment.SceneEnvironment)
			{
				Ref<TextureCube> tex = Renderer::GetBlackTextureCube();
				m_LightEnvironment.SceneEnvironment = Ref<Environment>::Create(tex, tex);
			}
		}

		// Directional Light
		{
			auto entities = GetAllEntitysWith<DirectionalLightComponent>();
			for (auto ent : entities)
			{
				if (m_LightEnvironment.DirectionalLightCount >= LightEnvironment::MaxDirectionLights)
					break;

				Entity entity{ ent, this };
				const DirectionalLightComponent& dirLight = entity.GetComponent<DirectionalLightComponent>();
				m_LightEnvironment.DirectionalLights[m_LightEnvironment.DirectionalLightCount++] = {
					dirLight.Radiance,
					glm::quat(GetWorldSpaceTransform(entity).Rotation) * glm::vec3(0, 1, 0),
					dirLight.Intensity
				};
			}
		}

		// Point Lights
		{
			auto pointLights = m_Registry.group<PointLightComponent>();
			m_LightEnvironment.PointLights.resize(pointLights.size());
			uint32_t pointLightIndex = 0;
			for (auto ent : pointLights)
			{
				Entity entity{ ent, this };
				auto& lightComponent = pointLights.get<PointLightComponent>(ent);
				auto worldTransform = GetWorldSpaceTransform(entity);
				m_LightEnvironment.PointLights[pointLightIndex++] = {
					worldTransform.Translation,
					lightComponent.Intensity,
					lightComponent.Radiance,
					lightComponent.Radius,
					lightComponent.Falloff
				};
			}
		}

		renderer->SetScene(this);
		renderer->BeginScene(camera);

		// Meshes
		{
			auto group = m_Registry.group<MeshComponent>(entt::get<TransformComponent>);
			for (auto ent : group)
			{
				auto [transformComponent, meshComponent] = group.get<TransformComponent, MeshComponent>(ent);
				if (!meshComponent.Visible)
					continue;

				SK_PERF_SCOPED("Scene Submit Mesh");
				if (AssetManager::IsValidAssetHandle(meshComponent.Mesh))
				{
					AsyncLoadResult meshResult = AssetManager::GetAssetAsync<Mesh>(meshComponent.Mesh);
					if (meshResult.Ready)
					{
						AsyncLoadResult meshSourceResult = AssetManager::GetAssetAsync<MeshSource>(meshResult.Asset->GetMeshSource());
						if (meshSourceResult.Ready)
						{
							Entity entity = { ent, this };
							glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);

							Ref<MaterialAsset> material;
							if (AssetManager::IsValidAssetHandle(meshComponent.Material))
							{
								AsyncLoadResult<MaterialAsset> result = AssetManager::GetAssetAsync<MaterialAsset>(meshComponent.Material);
								if (result.Ready)
									material = result.Asset;
							}
							
							if (!material)
							{
								const auto& submesh = meshSourceResult.Asset->GetSubmeshes()[meshComponent.SubmeshIndex];
								AssetHandle materialHandle = meshSourceResult.Asset->GetMaterials()[submesh.MaterialIndex];

								// NOTE(moro): this material should always be a memory asset so an async call is not necessary.
								//             just in case down the line something changes and it is not loaded a blocking call
								//             prevents the call to SubmitMesh from crashing
								material = AssetManager::GetAsset<MaterialAsset>(materialHandle);
							}

							renderer->SubmitMesh(meshResult.Asset, meshSourceResult.Asset, meshComponent.SubmeshIndex, material, transform, (int)ent);
						}
					}
				}
			}
		}

		renderer->EndScene();

		// Renderer 2D
		Ref<Renderer2D> renderer2D = renderer->GetRenderer2D();
		renderer2D->BeginScene(camera.Projection * camera.View);

		// Text
		{
			auto group = m_Registry.group<TextRendererComponent>(entt::get<TransformComponent>);
			for (auto ent : group)
			{
				auto [transformComponent, textComponent] = group.get<TransformComponent, TextRendererComponent>(ent);
				if (!AssetManager::IsValidAssetHandle(textComponent.FontHandle))
					continue;

				AsyncLoadResult fontResult = AssetManager::GetAssetAsync<Font>(textComponent.FontHandle);
				if (fontResult.Ready)
				{
					Entity entity = { ent, this };
					glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
					renderer2D->DrawString(textComponent.Text, fontResult.Asset, transform, textComponent.Kerning, textComponent.LineSpacing, textComponent.Color, (int)ent);
				}
			}
		}

		// Quads
		{
			auto entities = GetAllEntitysWith<SpriteRendererComponent>();
			for (auto ent : entities)
			{
				Entity entity(ent, this);
				const SpriteRendererComponent& spriteRendererComponent = entity.GetComponent<SpriteRendererComponent>();

				glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);

				Ref<Texture2D> texture = Renderer::GetWhiteTexture();
				if (AssetManager::IsValidAssetHandle(spriteRendererComponent.TextureHandle))
				{
					AsyncLoadResult<Texture2D> result = AssetManager::GetAssetAsync<Texture2D>(spriteRendererComponent.TextureHandle);
					if (result.Ready)
						texture = result.Asset;
				}

				renderer2D->DrawQuad(transform, texture, spriteRendererComponent.TilingFactor, spriteRendererComponent.Color, (int)ent);
			}
		}

		// Circles
		{
			auto entities = GetAllEntitysWith<CircleRendererComponent>();
			for (auto ent : entities)
			{
				Entity entity(ent, this);
				const CircleRendererComponent& circleRendererComponent = entity.GetComponent<CircleRendererComponent>();

				glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);

				if (circleRendererComponent.Filled)
					renderer2D->DrawFilledCircle(transform, circleRendererComponent.Color, circleRendererComponent.Thickness, circleRendererComponent.Fade, (int)ent);
				else
					renderer2D->DrawCircle(transform, circleRendererComponent.Color, (int)ent);
			}
		}

		renderer2D->EndScene();
	}

	Entity Scene::CloneEntity(Entity srcEntity)
	{
		SK_PROFILE_FUNCTION();
		
		Entity newEntity = CreateEntity();

		Ref<Scene> srcScene = srcEntity.GetScene().GetRef();
		ForEach(AllComponents, [&]<typename TComponent>
		{
			CopyComponentIfExists<TComponent>(srcEntity, srcScene->m_Registry, newEntity, m_Registry);
		});

		return newEntity;
	}

	Entity Scene::CreateEntity(const std::string& tag)
	{
		SK_PROFILE_FUNCTION();
		
		return CreateEntityWithUUID(UUID::Generate(), tag);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& tag)
	{
		SK_PROFILE_FUNCTION();
		
		if (uuid == UUID::Invalid)
			uuid = UUID::Generate();

		Entity entity{ m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TagComponent>(tag.empty() ? "new Entity" : tag);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<RelationshipComponent>();

		SK_CORE_VERIFY(!m_EntityUUIDMap.contains(uuid));
		m_EntityUUIDMap[uuid] = entity;
		return entity;
	}

	Entity Scene::CreateChildEntity(Entity parent, const std::string& tag)
	{
		return CreateChildEntityWithUUID(parent, UUID::Generate(), tag);
	}

	Entity Scene::CreateChildEntityWithUUID(Entity parent, UUID uuid, const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		if (uuid == UUID::Invalid)
			uuid = UUID::Generate();

		Entity entity = CreateEntityWithUUID(uuid, tag);

		if (parent)
			entity.SetParent(parent);

		return entity;
	}

	void Scene::DestroyEntity(Entity entity, bool destroyChildren)
	{
		DestroyEntityInternal(entity, destroyChildren, true);
	}

	Entity Scene::InstantiateMesh(Ref<Mesh> mesh)
	{
		Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource());
		if (!meshSource)
			return {};

		Entity entity = CreateEntity(meshSource->GetName());
		InstantiateSubmesh(mesh, meshSource, meshSource->GetRootNode(), entity);
		return entity;
	}

	void Scene::InstantiateSubmesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, const MeshNode& node, Entity parent)
	{
		Entity entity = CreateChildEntity(parent, node.Name);

		if (!node.Submeshes.empty())
		{
			MeshComponent& meshComp = entity.AddComponent<MeshComponent>();
			meshComp.Mesh = mesh->Handle;
			meshComp.SubmeshIndex = node.Submeshes[0];
		}

		for (const auto& childNodeIndex : node.Children)
		{
			const MeshNode& childNode = meshSource->GetNodes()[childNodeIndex];
			InstantiateSubmesh(mesh, meshSource, childNode, entity);
		}
	}

#if 0
	Entity Scene::InstantiateMesh(Ref<Mesh> mesh)
	{
		Ref<MeshSource> meshSource = mesh->GetMeshSource();
		Entity entity = CreateEntity(meshSource->GetName());
		auto& meshComponent = entity.AddComponent<MeshComponent>();
		meshComponent.Mesh = mesh->Handle;
		meshComponent.MaterialTable->GetMaterialCount() = meshSource->GetMaterials().size();
	}

	void Scene::InstantiateSubMesh(Ref<Mesh> mesh, const MeshNode& node, Entity parent)
	{
		Entity entity = CreateChildEntity(parent, node.Name);
		SubmeshComponent& submeshComponent = entity.AddComponent<SubmeshComponent>();
		submeshComponent.MeshEntity = entity.GetUUID();
		submeshComponent.SubmeshIndex = node.Submeshes;

		for (uint32_t childIndex : node.Children)
		{
			Ref<MeshSource> meshSource = mesh->GetMeshSource();
			const auto& childNodes = meshSource->GetNodes();
			InstantiateSubMesh(mesh, childNodes[childIndex], entity);
		}
	}
#endif

	void Scene::DestroyEntityInternal(Entity entity, bool destroyChildren, bool first)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_ASSERT(entity);
		if (!entity)
			return;

		SK_CORE_VERIFY(m_EntityUUIDMap.contains(entity.GetUUID()));

		if (!m_IsEditorScene)
		{
			if (entity.AllOf<RigidBody2DComponent>())
			{
				auto& rb = entity.GetComponent<RigidBody2DComponent>();
				SK_CORE_ASSERT(rb.RuntimeBody);
				m_PhysicsScene.GetWorld()->DestroyBody(rb.RuntimeBody);
				rb.RuntimeBody = nullptr;

				if (auto comp = entity.TryGetComponent<BoxCollider2DComponent>()) comp->RuntimeCollider = nullptr;
				if (auto comp = entity.TryGetComponent<CircleCollider2DComponent>()) comp->RuntimeCollider = nullptr;
			}

			if (entity.AllOf<ScriptComponent>())
				ScriptEngine::OnEntityDestroyed(entity);

		}

		if (m_ActiveCameraUUID == entity.GetUUID())
			m_ActiveCameraUUID = UUID::Invalid;

		if (first)
		{
			entity.RemoveParent();
			if (!destroyChildren)
				entity.RemoveChildren();
		}

		if (destroyChildren)
		{
			std::vector<UUID> children = entity.Children();
			for (auto& childID : children)
			{
				Entity child = m_EntityUUIDMap.at(childID);
				DestroyEntityInternal(child, destroyChildren, false);
			}
		}

		const UUID uuid = entity.GetUUID();
		m_Registry.destroy(entity);
		m_EntityUUIDMap.erase(uuid);
	}

	Entity Scene::TryGetEntityByUUID(UUID uuid) const
	{
		if (m_EntityUUIDMap.contains(uuid))
			return m_EntityUUIDMap.at(uuid);
		return Entity{};
	}

	Entity Scene::FindEntityByTag(const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		auto view = m_Registry.view<TagComponent>();
		for (auto& ent : view)
		{
			auto& comp = view.get<TagComponent>(ent);
			if (comp.Tag == tag)
				return { ent, this };
		}
		return Entity{};
	}

	Entity Scene::FindChildEntityByName(Entity entity, const std::string& name, bool recusive)
	{
		for (UUID id : entity.Children())
		{
			Entity child = m_EntityUUIDMap.at(id);
			if (child.GetName() == name)
				return child;

			if (recusive)
				FindChildEntityByName(child, name, recusive);
		}

		return Entity{};
	}

	bool Scene::IsValidEntity(Entity entity)const
	{
		return m_Registry.valid(entity);
	}

	bool Scene::ValidEntityID(UUID entityID) const
	{
		return entityID != UUID::Invalid && m_EntityUUIDMap.contains(entityID);
	}

	Entity Scene::GetActiveCameraEntity() const
	{
		return TryGetEntityByUUID(m_ActiveCameraUUID);
	}

	bool Scene::IsActiveCamera(Entity entity) const
	{
		if (!m_ActiveCameraUUID || !entity.AllOf<CameraComponent>())
			return false;

		return m_ActiveCameraUUID == entity.GetUUID();
	}

	void Scene::SetActiveCamera(Entity entity)
	{
		if (!entity.AllOf<CameraComponent>())
			return;

		m_ActiveCameraUUID = entity.GetUUID();
	}

	void Scene::ResizeCameras(float width, float height)
	{
		SK_PROFILE_FUNCTION();
		
		auto entities = GetAllEntitysWith<CameraComponent>();
		for (auto entityID : entities)
		{
			auto& component = entities.get<CameraComponent>(entityID);
			component.AspectRatio = width / height;
			component.Recalculate();
		}

	}

	void Scene::SetViewportSize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		if (m_ViewportWidth != 0 && m_ViewportHeight != 0)
			ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity) const
	{
		if (entity.HasParent())
			return GetWorldSpaceTransformMatrix(entity.Parent()) * entity.CalcTransform();
		return entity.CalcTransform();
	}

	TransformComponent Scene::GetWorldSpaceTransform(Entity entity)
	{
		if (!entity.HasParent())
			return entity.Transform();

		glm::mat4 worldSpaceTransform = GetWorldSpaceTransformMatrix(entity);
		TransformComponent transform;
		Math::DecomposeTransform(worldSpaceTransform, transform.Translation, transform.Rotation, transform.Scale);
		return transform;
	}

	bool Scene::ConvertToLocaSpace(Entity entity, glm::mat4& transformMatrix)
	{
		if (!entity.HasParent())
			return true;

		transformMatrix = glm::inverse(GetWorldSpaceTransformMatrix(entity.Parent())) * transformMatrix;
		return true;
	}

	bool Scene::ConvertToWorldSpace(Entity entity, glm::mat4& transformMatrix)
	{
		if (!entity.HasParent())
			return true;

		transformMatrix = GetWorldSpaceTransformMatrix(entity.Parent()) * transformMatrix;
		return true;
	}

	std::vector<Entity> Scene::GetEntitiesSorted()
	{
		std::vector<Entity> entities;
		entities.reserve(m_EntityUUIDMap.size());
		{
			auto view = m_Registry.view<IDComponent>();
			for (auto ent : view)
				entities.emplace_back(ent, this);

			std::sort(entities.begin(), entities.end(), [](Entity lhs, Entity rhs)
			{
				return lhs.GetUUID() < rhs.GetUUID();
			});
		}
		return entities;
	}

	bool Scene::ConvertToLocaSpace(Entity entity, TransformComponent& transform)
	{
		if (!entity.HasParent())
			return true;

		glm::mat4 localTransformMatrix = glm::inverse(GetWorldSpaceTransformMatrix(entity.Parent())) * transform.CalcTransform();
		return Math::DecomposeTransform(localTransformMatrix, transform.Translation, transform.Rotation, transform.Scale);
	}

	bool Scene::ConvertToWorldSpace(Entity entity, TransformComponent& transform)
	{
		if (!entity.HasParent())
			return true;

		glm::mat4 worldTransformMatrix = GetWorldSpaceTransformMatrix(entity);
		return Math::DecomposeTransform(worldTransformMatrix, transform.Translation, transform.Rotation, transform.Scale);
	}

	bool Scene::ConvertToLocaSpace(Entity entity)
	{
		return ConvertToLocaSpace(entity, entity.Transform());
	}

	bool Scene::ConvertToWorldSpace(Entity entity)
	{
		return ConvertToWorldSpace(entity, entity.Transform());
	}

	void Scene::OnPhysics2DPlay(bool connectWithScriptingAPI)
	{
		SK_PROFILE_FUNCTION();

		m_ContactListener.SetContext(this);
		m_PhysicsScene.CreateScene();
		b2World* world = m_PhysicsScene.GetWorld();

		if (connectWithScriptingAPI)
			world->SetContactListener(&m_ContactListener);

		m_PhysicsScene.SetOnPhyicsStepCallback([instance = Ref(this)](TimeStep fixedTimeStep) { instance->OnPhyicsStep(fixedTimeStep); });

		// Add missing RigidBodys
		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				if (!m_Registry.all_of<RigidBody2DComponent>(entity))
				{
					auto& rb = m_Registry.emplace<RigidBody2DComponent>(entity);
					rb.Type = RigidBody2DComponent::BodyType::Static;
				}
			}
		}

		// Add missing RigidBodys
		{
			auto view = m_Registry.view<CircleCollider2DComponent>();
			for (auto entity : view)
			{
				if (!m_Registry.all_of<RigidBody2DComponent>(entity))
				{
					auto& rb = m_Registry.emplace<RigidBody2DComponent>(entity);
					rb.Type = RigidBody2DComponent::BodyType::Static;
				}
			}
		}

		// Setup Box2D side
		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto e : view)
			{
				Entity entity{ e, this };
				auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
				glm::mat4 worldTransform = GetWorldSpaceTransformMatrix(entity);
				TransformComponent transform;
				Math::DecomposeTransform(worldTransform, transform.Translation, transform.Rotation, transform.Scale);
				//auto& transform = entity.Transform();

				//glm::mat4 localToWorld = entity.CalcLocalToWorldTransform();
				//glm::vec4 translation = localToWorld * glm::vec4(transform.Translation, 1.0f);
				//glm::vec4 rotation = localToWorld * glm::vec4(transform.Rotation, 0.0f);

				b2BodyDef bodydef;
				bodydef.type = SharkBodyTypeToBox2D(rb2d.Type);
				bodydef.position = { transform.Translation.x, transform.Translation.y };
				bodydef.angle = transform.Rotation.z;
				bodydef.fixedRotation = rb2d.FixedRotation;
				bodydef.bullet = rb2d.IsBullet;
				bodydef.awake = rb2d.Awake;
				bodydef.enabled = rb2d.Enabled;
				bodydef.gravityScale = rb2d.GravityScale;
				bodydef.allowSleep = rb2d.AllowSleep;
				bodydef.userData.pointer = (uintptr_t)entity.GetUUID().Value();

				rb2d.RuntimeBody = world->CreateBody(&bodydef);

				if (entity.AllOf<BoxCollider2DComponent>())
				{
					auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

					b2PolygonShape shape;
					shape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y, { bc2d.Offset.x, bc2d.Offset.y }, bc2d.Rotation);

					b2FixtureDef fixturedef;
					fixturedef.shape = &shape;
					fixturedef.friction = bc2d.Friction;
					fixturedef.density = bc2d.Density;
					fixturedef.restitution = bc2d.Restitution;
					fixturedef.restitutionThreshold = bc2d.RestitutionThreshold;
					fixturedef.isSensor = bc2d.IsSensor;

					bc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
				}

				if (entity.AllOf<CircleCollider2DComponent>())
				{
					auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

					b2CircleShape shape;
					shape.m_radius = cc2d.Radius * transform.Scale.x;
					shape.m_p = { cc2d.Offset.x, cc2d.Offset.y };

					b2FixtureDef fixturedef;
					fixturedef.shape = &shape;
					fixturedef.friction = cc2d.Friction;
					fixturedef.density = cc2d.Density;
					fixturedef.restitution = cc2d.Restitution;
					fixturedef.restitutionThreshold = cc2d.RestitutionThreshold;
					fixturedef.isSensor = cc2d.IsSensor;

					cc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
				}

			}

			{
				auto view = m_Registry.view<DistanceJointComponent>();
				for (entt::entity ent : view)
				{
					Entity entity{ ent, this };
					auto& distanceJointComp = entity.GetComponent<DistanceJointComponent>();

					if (!ValidEntityID(distanceJointComp.ConnectedEntity))
						continue;

					Entity connectedEntity = TryGetEntityByUUID(distanceJointComp.ConnectedEntity);
					SK_CORE_ASSERT(connectedEntity);

					auto connectedBody = m_PhysicsScene.GetBody(connectedEntity);
					auto body = m_PhysicsScene.GetBody(entity);
					if (!body || !connectedBody)
						continue;

					b2DistanceJointDef def;
					const b2Vec2 anchorA = body->GetPosition() + Phyiscs2DUtils::ToB2Vec(distanceJointComp.AnchorOffsetA);
					const b2Vec2 anchorB = connectedBody->GetPosition() + Phyiscs2DUtils::ToB2Vec(distanceJointComp.AnchorOffsetB);

					def.Initialize(body, connectedBody, anchorA, anchorB);
					def.collideConnected = distanceJointComp.CollideConnected;

					if (distanceJointComp.MinLength >= 0.0f)
						def.minLength = distanceJointComp.MinLength;

					if (distanceJointComp.MaxLength >= 0.0f)
						def.maxLength = distanceJointComp.MaxLength;

					def.stiffness = distanceJointComp.Stiffness;
					def.damping = distanceJointComp.Damping;

					b2Joint* joint = world->CreateJoint(&def);
					distanceJointComp.RuntimeJoint = (b2DistanceJoint*)joint;
				}
			}

			{
				auto view = m_Registry.view<HingeJointComponent>();
				for (entt::entity ent : view)
				{
					Entity entity{ ent, this };
					auto& hingeJointComp = entity.GetComponent<HingeJointComponent>();

					if (!ValidEntityID(hingeJointComp.ConnectedEntity))
						continue;

					Entity connectedEntity = TryGetEntityByUUID(hingeJointComp.ConnectedEntity);
					SK_CORE_ASSERT(connectedEntity);

					auto connectedBody = m_PhysicsScene.GetBody(connectedEntity);
					auto body = m_PhysicsScene.GetBody(entity);
					if (!body || !connectedBody)
						continue;

					b2RevoluteJointDef def;
					def.Initialize(body, connectedBody, Phyiscs2DUtils::ToB2Vec(hingeJointComp.Anchor));
					def.collideConnected = hingeJointComp.CollideConnected;

					def.lowerAngle = hingeJointComp.LowerAngle;
					def.upperAngle = hingeJointComp.UpperAngle;
					def.enableMotor = hingeJointComp.EnableMotor;
					def.motorSpeed = hingeJointComp.MotorSpeed;
					def.maxMotorTorque = hingeJointComp.MaxMotorTorque;

					b2Joint* joint = world->CreateJoint(&def);
					hingeJointComp.RuntimeJoint = (b2RevoluteJoint*)joint;
				}
			}

			{
				auto view = m_Registry.view<PrismaticJointComponent>();
				for (entt::entity ent : view)
				{
					Entity entity{ ent, this };
					auto& component = entity.GetComponent<PrismaticJointComponent>();

					if (!ValidEntityID(component.ConnectedEntity))
						continue;

					Entity connectedEntity = TryGetEntityByUUID(component.ConnectedEntity);
					SK_CORE_ASSERT(connectedEntity);

					auto connectedBody = m_PhysicsScene.GetBody(connectedEntity);
					auto body = m_PhysicsScene.GetBody(entity);
					if (!body || !connectedBody)
						continue;

					b2PrismaticJointDef def;
					def.Initialize(body, connectedBody, Phyiscs2DUtils::ToB2Vec(component.Anchor), Phyiscs2DUtils::ToB2Vec(component.Axis));
					def.collideConnected = component.CollideConnected;

					def.enableLimit = component.EnableLimit;
					def.lowerTranslation = component.LowerTranslation;
					def.upperTranslation = component.UpperTranslation;
					def.enableMotor = component.EnableMotor;
					def.motorSpeed = component.MotorSpeed;
					def.maxMotorForce = component.MaxMotorForce;

					b2Joint* joint = world->CreateJoint(&def);
					component.RuntimeJoint = (b2PrismaticJoint*)joint;
				}
			}

			{
				auto view = m_Registry.view<PulleyJointComponent>();
				for (entt::entity ent : view)
				{
					Entity entity{ ent, this };
					auto& component = entity.GetComponent<PulleyJointComponent>();

					if (!ValidEntityID(component.ConnectedEntity))
						continue;

					Entity connectedEntity = TryGetEntityByUUID(component.ConnectedEntity);
					SK_CORE_ASSERT(connectedEntity);

					auto connectedBody = m_PhysicsScene.GetBody(connectedEntity);
					auto body = m_PhysicsScene.GetBody(entity);
					if (!body || !connectedBody)
						continue;

					b2PulleyJointDef def;
					def.Initialize(body, connectedBody,
								   Phyiscs2DUtils::ToB2Vec(component.GroundAnchorA),
								   Phyiscs2DUtils::ToB2Vec(component.GroundAnchorB),
								   body->GetPosition() + Phyiscs2DUtils::ToB2Vec(component.AnchorA),
								   connectedBody->GetPosition() + Phyiscs2DUtils::ToB2Vec(component.AnchorB),
								   component.Ratio);

					def.collideConnected = component.CollideConnected;

					b2Joint* joint = world->CreateJoint(&def);
					component.RuntimeJoint = (b2PulleyJoint*)joint;
				}
			}
		}
	}

	void Scene::OnPhysics2DStop()
	{
		m_PhysicsScene.SetOnPhyicsStepCallback(nullptr);
		m_PhysicsScene.DestoryScene();
		m_ContactListener.SetContext(nullptr);
	}

	void Scene::OnPhyicsStep(TimeStep fixedTimeStep)
	{
		SK_PROFILE_FUNCTION();

		for (auto& [uuid, gcHandle] : ScriptEngine::GetEntityInstances())
			MethodThunks::OnPhysicsUpdate(gcHandle, fixedTimeStep);

	}

	void Scene::OnRigidBody2DComponentCreated(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		auto& transform = entity.Transform();

		b2BodyDef bodydef;
		bodydef.type = SharkBodyTypeToBox2D(rb2d.Type);
		bodydef.position = { transform.Translation.x, transform.Translation.y };
		bodydef.angle = transform.Rotation.z;
		bodydef.fixedRotation = rb2d.FixedRotation;
		bodydef.userData.pointer = (uintptr_t)entity.GetUUID().Value();

		rb2d.RuntimeBody = m_PhysicsScene.GetWorld()->CreateBody(&bodydef);
	}

	void Scene::OnBoxCollider2DComponentCreated(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		auto& transform = entity.Transform();
		auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

		b2PolygonShape shape;
		shape.SetAsBox(bc2d.Size.x * transform.Scale.x, bc2d.Size.y * transform.Scale.y, { bc2d.Offset.x, bc2d.Offset.y }, bc2d.Rotation);

		b2FixtureDef fixturedef;
		fixturedef.shape = &shape;
		fixturedef.friction = bc2d.Friction;
		fixturedef.density = bc2d.Density;
		fixturedef.restitution = bc2d.Restitution;
		fixturedef.restitutionThreshold = bc2d.RestitutionThreshold;

		bc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
	}

	void Scene::OnCircleCollider2DComponentCreated(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		auto& transform = entity.Transform();
		auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

		b2CircleShape shape;
		shape.m_radius = cc2d.Radius * transform.Scale.x;
		shape.m_p = { cc2d.Offset.x, cc2d.Offset.y };

		b2FixtureDef fixturedef;
		fixturedef.shape = &shape;
		fixturedef.friction = cc2d.Friction;
		fixturedef.density = cc2d.Density;
		fixturedef.restitution = cc2d.Restitution;
		fixturedef.restitutionThreshold = cc2d.RestitutionThreshold;

		cc2d.RuntimeCollider = rb2d.RuntimeBody->CreateFixture(&fixturedef);
	}

	void Scene::OnCameraComponentCreated(entt::registry& registry, entt::entity ent)
	{
		if (!m_ActiveCameraUUID)
			m_ActiveCameraUUID = registry.get<IDComponent>(ent).ID;

		if (m_ViewportWidth > 0.0f && m_ViewportHeight > 0.0f)
		{
			auto& component = registry.get<CameraComponent>(ent);
			component.AspectRatio = (float)m_ViewportWidth / (float)m_ViewportHeight;
			component.Recalculate();
		}
	}

	void Scene::OnRigidBody2DComponentDestroyed(entt::registry& registry, entt::entity ent)
	{
		Entity entity{ ent, this };
		auto& rb2d = entity.GetComponent<RigidBody2DComponent>();
		
		if (rb2d.RuntimeBody)
		{
			m_PhysicsScene.GetWorld()->DestroyBody(rb2d.RuntimeBody);
			rb2d.RuntimeBody = nullptr;
		}
	}

	void Scene::OnBoxCollider2DComponentDestroyed(entt::registry& registry, entt::entity ent)
	{
		Entity entity{ ent, this };
		auto& comp = entity.GetComponent<BoxCollider2DComponent>();

		if (comp.RuntimeCollider)
		{
			b2Body* body = comp.RuntimeCollider->GetBody();
			body->DestroyFixture(comp.RuntimeCollider);
			comp.RuntimeCollider = nullptr;
		}
	}

	void Scene::OnCircleCollider2DComponentDestroyed(entt::registry& registry, entt::entity ent)
	{
		Entity entity{ ent, this };
		auto& comp = entity.GetComponent<CircleCollider2DComponent>();

		if (comp.RuntimeCollider)
		{
			b2Body* body = comp.RuntimeCollider->GetBody();
			body->DestroyFixture(comp.RuntimeCollider);
			comp.RuntimeCollider = nullptr;
		}
	}

	void Scene::OnScriptComponentDestroyed(entt::registry& registry, entt::entity ent)
	{
		Entity entity{ ent, this };

		if (ScriptEngine::IsInstantiated(entity))
			ScriptEngine::DestroyEntityInstance(entity, true);
	}

	void ContactListener::BeginContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		b2Body* bodyA = fixtureA->GetBody();
		b2Body* bodyB = fixtureB->GetBody();

		UUID uuidA = (UUID)bodyA->GetUserData().pointer;
		UUID uuidB = (UUID)bodyB->GetUserData().pointer;

		Entity entityA = m_Context->TryGetEntityByUUID(uuidA);
		Entity entityB = m_Context->TryGetEntityByUUID(uuidB);

		b2Shape* shapeA = fixtureA->GetShape();
		b2Shape* shapeB = fixtureB->GetShape();

		Collider2DType colliderTypeA = shapeA->GetType() == b2Shape::e_polygon ? Collider2DType::BoxCollider : Collider2DType::CircleCollider;
		Collider2DType colliderTypeB = shapeB->GetType() == b2Shape::e_polygon ? Collider2DType::BoxCollider : Collider2DType::CircleCollider;

		ScriptGlue::CallCollishionBegin(entityA, entityB, colliderTypeB, fixtureB->IsSensor());
		ScriptGlue::CallCollishionBegin(entityB, entityA, colliderTypeA, fixtureA->IsSensor());
	}

	void ContactListener::EndContact(b2Contact* contact)
	{
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		b2Body* bodyA = fixtureA->GetBody();
		b2Body* bodyB = fixtureB->GetBody();

		UUID uuidA = (UUID)bodyA->GetUserData().pointer;
		UUID uuidB = (UUID)bodyB->GetUserData().pointer;

		Entity entityA = m_Context->TryGetEntityByUUID(uuidA);
		Entity entityB = m_Context->TryGetEntityByUUID(uuidB);

		b2Shape* shapeA = fixtureA->GetShape();
		b2Shape* shapeB = fixtureB->GetShape();

		Collider2DType colliderTypeA = shapeA->GetType() == b2Shape::e_polygon ? Collider2DType::BoxCollider : Collider2DType::CircleCollider;
		Collider2DType colliderTypeB = shapeB->GetType() == b2Shape::e_polygon ? Collider2DType::BoxCollider : Collider2DType::CircleCollider;

		ScriptGlue::CallCollishionEnd(entityA, entityB, colliderTypeB, fixtureB->IsSensor());
		ScriptGlue::CallCollishionEnd(entityB, entityA, colliderTypeA, fixtureA->IsSensor());
	}

	void ContactListener::SetContext(const Ref<Scene>& context)
	{
		m_Context = context;
	}

}
