#include "skpch.h"
#include "Scene.h"

#include "Shark/Core/SelectionManager.h"
#include "Shark/Asset/AssetManager.h"

#include "Shark/Scene/Entity.h"
#include "Shark/Render/Renderer.h"
#include "Shark/Render/SceneRenderer.h"

#include "Shark/Scripting/ScriptEngine.h"
#include "Shark/Scripting/ScriptGlue.h"

#include "Shark/Math/Math.h"
#include "Shark/File/FileSystem.h"

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

			Component& comp = srcRegistry.get<Component>(srcEntity);
			destRegistry.emplace_or_replace<Component>(destEntity, comp);
		}
	}

	template<typename Component>
	void CopyComponentIfExists(entt::entity srcEntity, entt::registry& srcRegistry, entt::entity destEntity, entt::registry& destRegistry)
	{
		SK_PROFILE_FUNCTION();

		if (Component* comp = srcRegistry.try_get<Component>(srcEntity))
			destRegistry.emplace_or_replace<Component>(destEntity, *comp);
	}


	Scene::Scene(const std::string& name)
		: m_Name(name)
	{
		m_Registry.on_construct<CameraComponent>().connect<&Scene::OnCameraComponentCreated>(this);
	}

	Scene::~Scene()
	{
		if (m_IsRunning)
			OnSceneStop();

		if (m_PhysicsScene)
			OnPhysics2DStop();

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
			destScene->m_EntityUUIDMap[uuid] = destScene->CreateEntityWithUUID(uuid, srcEntity.GetName(), false);
			srcEntity.RemoveComponentIsExists<Internal::RootParentComponent>();
		}

		m_ScriptStorage.CopyTo(destScene->m_ScriptStorage);

		ForEach(CopySceneComponents, [&]<typename TComp>()
		{
			CopyComponents<TComp>(m_Registry, destRegistry, destScene->m_EntityUUIDMap);
		});

		destScene->SortEntitites();
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

		SK_CORE_INFO_TAG("Scene", "Scene Play '{}'", Handle);

		m_IsRunning = true;

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
				auto& scriptEngine = ScriptEngine::Get();

				auto view = m_Registry.view<ScriptComponent>();
				for (auto ent : view)
				{
					Entity entity{ ent, this };
					UUID entityID = entity.GetUUID();

					auto& scriptComponent = entity.GetComponent<ScriptComponent>();
					if (!scriptEngine.IsValidScriptID(scriptComponent.ScriptID))
					{
						SK_CORE_WARN_TAG("Scripting", "Entity {} has an invalid script '{}'", entityID, scriptComponent.ScriptID);
						continue;
					}

					if (!m_ScriptStorage.EntityInstances.contains(entityID))
					{
						SK_CORE_ERROR_TAG("Scripting", "Entity {} isn't in script storage", entityID);
						continue;
					}

					scriptComponent.Instance = scriptEngine.Instantiate(entity.GetUUID(), m_ScriptStorage);
				}

				for (const auto& [entityID, script] : m_ScriptStorage.EntityInstances)
				{
					Entity entity = TryGetEntityByUUID(entityID);
					auto& scriptComponent = entity.GetComponent<ScriptComponent>();
					if (scriptComponent.OnCreateCalled)
						continue;

					script.Instance.InvokeMethod("OnCreate");
					scriptComponent.OnCreateCalled = true;
				}

			}

		}

		// Setup Cameras
		bool activeCameraFound = false;
		if (HasActiveCamera(true))
		{
			Entity activeCamera = GetEntityByID(m_ActiveCameraUUID);
			if (activeCamera.HasComponent<CameraComponent>())
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

		SK_CORE_INFO_TAG("Scene", "Scene Stop '{}'", Handle);

		m_IsRunning = false;

		for (auto& [entityID, script] : m_ScriptStorage.EntityInstances)
		{
			auto& scriptEngine = ScriptEngine::Get();
			scriptEngine.Destoy(entityID, m_ScriptStorage);
		}

		OnPhysics2DStop();

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

		{
			auto entities = GetAllEntitysWith<ScriptComponent>();
			for (auto entity : entities)
			{
				const auto& scriptComponent = entities.get<ScriptComponent>(entity);
				if (scriptComponent.Instance)
				{
					scriptComponent.Instance->InvokeMethod("OnUpdate", (float)ts);
				}
			}
		}

		{
			m_PhysicsScene->Step(ts);

			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto e : view)
			{
				Entity entity{ e, this };
				const auto& rb2d = view.get<RigidBody2DComponent>(entity);
				auto& transform = entity.Transform();
				if (entity.HasParent())
				{
					glm::mat4 localTransform = glm::inverse(GetWorldSpaceTransformMatrix(entity.Parent())) * Physics2DUtils::GetMatrix(rb2d.RuntimeBody);
					TransformComponent localBodyTransform;
					Math::DecomposeTransform(localTransform, localBodyTransform.Translation, localBodyTransform.Rotation, localBodyTransform.Scale);
					transform.Translation.x = localBodyTransform.Translation.x;
					transform.Translation.y = localBodyTransform.Translation.y;
					transform.Rotation.z = localBodyTransform.Rotation.z;
					continue;
				}

				const auto& pos = rb2d.RuntimeBody->GetPosition();
				transform.Translation.xy = Physics2DUtils::FromBody(rb2d.RuntimeBody);
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

		m_PhysicsScene->Step(ts);

		auto view = m_Registry.view<RigidBody2DComponent>();
		for (auto e : view)
		{
			Entity entity{ e, this };
			const auto& rb2d = view.get<RigidBody2DComponent>(entity);
			auto& transform = entity.Transform();
			if (entity.HasParent())
			{
				glm::mat4 localTransform = glm::inverse(GetWorldSpaceTransformMatrix(entity.Parent())) * Physics2DUtils::GetMatrix(rb2d.RuntimeBody);
				TransformComponent localBodyTransform;
				Math::DecomposeTransform(localTransform, localBodyTransform.Translation, localBodyTransform.Rotation, localBodyTransform.Scale);
				transform.Translation.x = localBodyTransform.Translation.x;
				transform.Translation.y = localBodyTransform.Translation.y;
				transform.Rotation.z = localBodyTransform.Rotation.z;
				continue;
			}

			const auto& pos = rb2d.RuntimeBody->GetPosition();
			transform.Translation.xy = Physics2DUtils::FromBody(rb2d.RuntimeBody);
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
			auto entities = GetAllEntitysWith<SubmeshComponent>();
			for (auto ent : entities)
			{
				auto& submeshComponent = entities.get<SubmeshComponent>(ent);
				if (!submeshComponent.Visible)
					continue;

				SK_PERF_SCOPED("Scene Submit Mesh");
				if (AsyncLoadResult meshResult = AssetManager::GetAssetAsync<Mesh>(submeshComponent.Mesh))
				{
					if (AsyncLoadResult meshSourceResult = AssetManager::GetAssetAsync<MeshSource>(meshResult.Asset->GetMeshSource()))
					{
						Entity entity = { ent, this };
						glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);

						const auto& submeshes = meshSourceResult->GetSubmeshes();
						const auto& submesh = submeshes[submeshComponent.SubmeshIndex];

						AssetHandle materialHandle = submeshComponent.Material ? submeshComponent.Material : meshSourceResult->GetMaterials()[submesh.MaterialIndex];
						Ref<MaterialAsset> material = AssetManager::GetAssetAsync<MaterialAsset>(materialHandle);
						if (!material)
							continue;

						renderer->SubmitMesh(meshResult.Asset, meshSourceResult.Asset, submeshComponent.SubmeshIndex, material, transform, (int)ent);
						if (SelectionManager::IsEntityOrAncestorSelected(entity))
							renderer->SubmitSelectedMesh(meshResult.Asset, meshSourceResult.Asset, submeshComponent.SubmeshIndex, material, transform);
					}
				}
			}
		}

		// Static Meshes
		{
			auto entities = GetAllEntitysWith<StaticMeshComponent>();
			for (auto ent : entities)
			{
				auto& meshComponent = entities.get<StaticMeshComponent>(ent);
				if (!meshComponent.Visible)
					continue;

				Entity entity{ ent, this };
				const bool isSelected = SelectionManager::IsEntityOrAncestorSelected(entity);

				SK_PERF_SCOPED("Scene Submit Static Mesh");
				if (auto mesh = AssetManager::GetAssetAsync<Mesh>(meshComponent.StaticMesh))
				{
					if (auto meshSource = AssetManager::GetAssetAsync<MeshSource>(mesh->GetMeshSource()))
					{
						glm::mat4 rootTransform = GetWorldSpaceTransformMatrix(entity);

						const auto& submeshes = meshSource->GetSubmeshes();
						const auto& nodes = meshSource->GetNodes();
						for (const auto& node : nodes)
						{
							for (const auto& submeshIndex : node.Submeshes)
							{
								const auto& submesh = submeshes[submeshIndex];
								auto materialHandle = meshComponent.MaterialTable->HasMaterial(submesh.MaterialIndex) ? meshComponent.MaterialTable->GetMaterial(submesh.MaterialIndex) : mesh->GetMaterials()->GetMaterial(submesh.MaterialIndex);
								auto material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

								renderer->SubmitMesh(mesh, meshSource, submeshIndex, material, rootTransform * node.Transform, (int)ent);
								if (isSelected)
									renderer->SubmitSelectedMesh(mesh, meshSource, submeshIndex, material, rootTransform * node.Transform);
							}
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

	Entity Scene::CloneEntity(Entity entity, bool cloneChildren)
	{
		SK_PROFILE_FUNCTION();

		Entity newEntity = CreateEntity();

		Ref<Scene> srcScene = entity.m_Scene.GetRef();
		CopyComponentIfExists<TagComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<TransformComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		//CopyComponentIfExists<RelationshipComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<SpriteRendererComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<CircleRendererComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<TextRendererComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<MeshComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<MeshFilterComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<SubmeshComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<StaticMeshComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<PointLightComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<DirectionalLightComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<SkyComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<CameraComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<RigidBody2DComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<BoxCollider2DComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<CircleCollider2DComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<DistanceJointComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<HingeJointComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<PrismaticJointComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<PulleyJointComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<ScriptComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);
		//CopyComponentIfExists<Internal::RootParentComponent>(entity, srcScene->m_Registry, newEntity, m_Registry);

		if (newEntity.HasComponent<ScriptComponent>())
		{
			// #TODO(moro): setup entity storage
			const auto& scriptComponent = newEntity.GetComponent<ScriptComponent>();
			m_ScriptStorage.SetupEntityStorage(scriptComponent.ScriptID, newEntity.GetUUID());
			m_ScriptStorage.CopyEntityStorage(entity.GetUUID(), newEntity.GetUUID(), m_ScriptStorage);
		}

		for (auto childID : entity.Children())
		{
			Entity childDuplicate = CloneEntity(GetEntityByID(childID));
			childDuplicate.SetParent(newEntity);
		}

		newEntity.SetParent(entity.Parent());

		if (m_IsRunning)
		{
			if (newEntity.HasComponent<ScriptComponent>())
			{
				const auto& scriptComponent = newEntity.GetComponent<ScriptComponent>();
				auto& scriptEngine = ScriptEngine::Get();
				scriptEngine.Instantiate(newEntity.GetUUID(), m_ScriptStorage);
			}
		}

		return newEntity;
	}

	Entity Scene::CreateEntity(const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		return CreateEntityWithUUID(UUID::Generate(), tag);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& tag, bool shouldSort)
	{
		SK_PROFILE_FUNCTION();

		if (uuid == UUID::Invalid)
			uuid = UUID::Generate();

		Entity entity{ m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TagComponent>(tag.empty() ? "new Entity" : tag);
		entity.AddComponent<TransformComponent>();
		entity.AddComponent<RelationshipComponent>();
		entity.AddComponent<Internal::RootParentComponent>();

		if (shouldSort)
			SortEntitites();

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

	void Scene::SortEntitites()
	{
		m_Registry.sort<IDComponent>([](const entt::entity lhs, const entt::entity rhs)
		{
			return lhs < rhs;
		});
	}

	Entity Scene::InstantiateMesh(Ref<Mesh> mesh)
	{
		const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(mesh);
		Entity rootEntity = CreateEntity(FileSystem::GetStemString(metadata.FilePath));
		rootEntity.AddComponent<MeshComponent>(mesh->Handle);
		if (auto meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource()))
		{
			BuildMeshEntityHierarchy(rootEntity, mesh, meshSource->GetRootNode());
		}
		return rootEntity;
	}

	Entity Scene::InstantiateStaticMesh(Ref<Mesh> mesh)
	{
		const auto& metadata = Project::GetActiveEditorAssetManager()->GetMetadata(mesh);
		Entity rootEntity = CreateEntity(FileSystem::GetStemString(metadata.FilePath));
		rootEntity.AddComponent<StaticMeshComponent>(mesh->Handle);
		return rootEntity;
	}

	void Scene::RebuildMeshEntityHierarchy(Entity entity)
	{
		SK_CORE_ASSERT(entity.HasComponent<MeshComponent>(), "Attempted to Rebuild mesh entity hierachy for an entity that has no MeshComponent!");
		if (!entity.HasComponent<MeshComponent>())
			return;

		auto childIDs = entity.Children();
		for (auto childID : childIDs)
		{
			Entity child = TryGetEntityByUUID(childID);
			if (child && child.HasComponent<MeshFilterComponent>())
			{
				DestroyEntity(child);
			}
		}

		auto& meshComponent = entity.GetComponent<MeshComponent>();
		if (auto mesh = AssetManager::GetAsset<Mesh>(meshComponent.Mesh))
		{
			if (auto meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource()))
			{
				BuildMeshEntityHierarchy(entity, mesh, meshSource->GetRootNode());
			}
		}
	}

	Entity Scene::GetEntityByID(UUID id) const
	{
		return m_EntityUUIDMap.at(id);
	}

	void Scene::BuildMeshEntityHierarchy(Entity parent, Ref<Mesh> mesh, const MeshNode& node)
	{
		Ref<MeshSource> meshSource = AssetManager::GetAsset<MeshSource>(mesh->GetMeshSource());
		const auto& nodes = meshSource->GetNodes();

		if (node.IsRoot() && node.Submeshes.size() == 0)
		{
			for (uint32_t child : node.Children)
			{
				MeshNode childNode = nodes[child];
				childNode.LocalTransform = node.LocalTransform * childNode.LocalTransform;
				BuildMeshEntityHierarchy(parent, mesh, childNode);
			}
			return;
		}

		Entity nodeEntity = CreateChildEntity(parent, node.Name);
		nodeEntity.Transform().SetTransform(node.LocalTransform);
		nodeEntity.AddComponent<MeshFilterComponent>();

		if (node.Submeshes.size() == 1)
		{
			nodeEntity.AddComponent<SubmeshComponent>(mesh->Handle, node.Submeshes[0]);
		}
		else if (node.Submeshes.size() > 1)
		{
			for (uint32_t i = 0; i < node.Submeshes.size(); i++)
			{
				uint32_t submeshIndex = node.Submeshes[i];
				const auto& submesh = meshSource->GetSubmeshes()[submeshIndex];
				Entity childEntity = CreateChildEntity(nodeEntity, submesh.MeshName);

				childEntity.AddComponent<MeshFilterComponent>();
				childEntity.AddComponent<SubmeshComponent>(mesh->Handle, submeshIndex);
			}
		}

		for (uint32_t child : node.Children)
		{
			BuildMeshEntityHierarchy(nodeEntity, mesh, nodes[child]);
		}
	}

	void Scene::DestroyEntityInternal(Entity entity, bool destroyChildren, bool first)
	{
		SK_PROFILE_FUNCTION();

		if (!entity)
			return;

		if (destroyChildren)
		{
			for (uint32_t i = 0; i < entity.Children().size(); i++)
			{
				auto childID = entity.Children()[i];
				Entity child = GetEntityByID(childID);
				DestroyEntityInternal(child, destroyChildren, false);
			}
		}

		const UUID id = entity.GetUUID();
		if (m_ActiveCameraUUID == id)
			m_ActiveCameraUUID = UUID::Invalid;

		if (SelectionManager::IsSelected(SelectionContext::Entity, id))
			SelectionManager::Unselect(SelectionContext::Entity, id);

		if (first)
		{
			if (entity.HasParent())
			{
				auto parent = entity.Parent();
				parent.RemoveChild(entity);
			}
		}

		entity.RemoveComponentIsExists<BoxCollider2DComponent>();
		entity.RemoveComponentIsExists<CircleCollider2DComponent>();
		entity.RemoveComponentIsExists<RigidBody2DComponent>();
		entity.RemoveComponentIsExists<ScriptComponent>();

		m_Registry.destroy(entity);
		m_EntityUUIDMap.erase(id);
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

	bool Scene::IsValidEntityID(UUID entityID) const
	{
		return entityID != UUID::Invalid && m_EntityUUIDMap.contains(entityID);
	}

	Entity Scene::GetActiveCameraEntity() const
	{
		return TryGetEntityByUUID(m_ActiveCameraUUID);
	}

	bool Scene::HasActiveCamera() const
	{
		return IsValidEntityID(m_ActiveCameraUUID);
	}

	bool Scene::HasActiveCamera(bool setIfAnyAvailable)
	{
		if (IsValidEntityID(m_ActiveCameraUUID))
			return true;

		if (!setIfAnyAvailable)
			return false;

		auto entities = m_Registry.view<CameraComponent>();
		if (!entities.empty() && m_Registry.valid(entities.front()))
		{
			auto& idComp = m_Registry.get<IDComponent>(entities.front());
			m_ActiveCameraUUID = idComp.ID;
		}

		return IsValidEntityID(m_ActiveCameraUUID);
	}

	bool Scene::IsActiveCamera(Entity entity) const
	{
		if (!m_ActiveCameraUUID || !entity.HasComponent<CameraComponent>())
			return false;

		return m_ActiveCameraUUID == entity.GetUUID();
	}

	void Scene::SetActiveCamera(Entity entity)
	{
		if (!entity.HasComponent<CameraComponent>())
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
			return GetWorldSpaceTransformMatrix(entity.Parent()) * entity.Transform().CalcTransform();
		return entity.Transform().CalcTransform();
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

	void Scene::ParentEntity(Entity entity, Entity parent)
	{
		if (parent.IsDescendantOf(entity))
		{
			UnparentEntity(parent);

			Entity newParnet = entity.Parent();
			if (newParnet)
			{
				ParentEntity(parent, newParnet);
				UnparentEntity(entity);
			}
		}
		else
		{
			UnparentEntity(entity);
		}

		entity.SetParent(parent);
		ConvertToLocaSpace(entity);
	}

	void Scene::UnparentEntity(Entity entity)
	{
		if (!entity.HasParent())
			return;

		ConvertToWorldSpace(entity);
		entity.SetParent({});
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

	void Scene::OnPhysics2DPlay(bool connectWithScriptingAPI)
	{
		SK_PROFILE_FUNCTION();

		SK_CORE_VERIFY(m_PhysicsScene == nullptr);
		m_PhysicsScene = sknew Physics2DScene(this, connectWithScriptingAPI);
		b2World* world = m_PhysicsScene->GetWorld();

		m_PhysicsScene->SetOnPhyicsStepCallback([instance = Ref(this)](TimeStep fixedTimeStep) { instance->OnPhysicsStep(fixedTimeStep); });

		// Add missing RigidBodys
		{
			auto view = m_Registry.view<BoxCollider2DComponent>();
			for (auto entity : view)
			{
				if (!m_Registry.all_of<RigidBody2DComponent>(entity))
				{
					auto& rb = m_Registry.emplace<RigidBody2DComponent>(entity);
					rb.Type = RigidbodyType::Static;
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
					rb.Type = RigidbodyType::Static;
				}
			}
		}

		// Setup Box2D side
		{
			auto view = m_Registry.view<RigidBody2DComponent>();
			for (auto e : view)
			{
				Entity entity{ e, this };
				auto transform = GetWorldSpaceTransform(entity);
				CreateRuntimeRigidBody2D(entity, transform);

				if (entity.HasComponent<BoxCollider2DComponent>())
					CreateRuntimeBoxCollider2D(entity, transform);

				if (entity.HasComponent<CircleCollider2DComponent>())
					CreateRuntimeCircleCollider2D(entity, transform);
			}

			{
				auto view = m_Registry.view<DistanceJointComponent>();
				for (entt::entity ent : view)
				{
					Entity entity{ ent, this };
					CreateRuntimeDistanceJoint2D(entity);
				}
			}

			{
				auto view = m_Registry.view<HingeJointComponent>();
				for (entt::entity ent : view)
				{
					Entity entity{ ent, this };
					CreateRuntimeHingeJoint2D(entity);
				}
			}

			{
				auto view = m_Registry.view<PrismaticJointComponent>();
				for (entt::entity ent : view)
				{
					Entity entity{ ent, this };
					CreateRuntimePrismaticJoint2D(entity);
				}
			}

			{
				auto view = m_Registry.view<PulleyJointComponent>();
				for (entt::entity ent : view)
				{
					Entity entity{ ent, this };
					CreateRuntimePulleyJoint2D(entity);
				}
			}
		}
	}

	void Scene::OnPhysics2DStop()
	{
		skdelete m_PhysicsScene;
		m_PhysicsScene = nullptr;
	}

	void Scene::OnPhysicsStep(TimeStep fixedTimeStep)
	{
		SK_PROFILE_FUNCTION();

		auto entities = GetAllEntitysWith<ScriptComponent>();
		for (auto ent : entities)
		{
			auto& scriptComponent = entities.get<ScriptComponent>(ent);
			if (scriptComponent.Instance)
				scriptComponent.Instance->InvokeMethod("OnPhysicsUpdate", (float)fixedTimeStep);
		}

	}

	void Scene::OnRigidBody2DComponentCreated(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		TransformComponent transform = GetWorldSpaceTransform(entity);
		CreateRuntimeRigidBody2D(entity, transform);

		// TODO(moro): should this create colliders
	}

	void Scene::OnBoxCollider2DComponentCreated(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		if (entity.HasComponent<RigidBody2DComponent>())
		{
			TransformComponent transform = GetWorldSpaceTransform(entity);
			CreateRuntimeBoxCollider2D(entity, transform);
		}
	}

	void Scene::OnCircleCollider2DComponentCreated(entt::registry& registry, entt::entity ent)
	{
		SK_CORE_ASSERT(!m_IsEditorScene);

		Entity entity{ ent, this };
		if (entity.HasComponent<RigidBody2DComponent>())
		{
			TransformComponent transform = GetWorldSpaceTransform(entity);
			CreateRuntimeCircleCollider2D(entity, transform);
		}
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
			m_PhysicsScene->GetWorld()->DestroyBody(rb2d.RuntimeBody);
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
		auto& idComponent = registry.get<IDComponent>(ent);

		auto& scriptEngine = ScriptEngine::Get();
		scriptEngine.Destoy(idComponent.ID, m_ScriptStorage);
	}

	void Scene::CreateRuntimeRigidBody2D(Entity entity, const TransformComponent& worldTransform)
	{
		auto& rigidBody = entity.GetComponent<RigidBody2DComponent>();

		b2BodyDef bodydef;
		bodydef.type = Physics2DUtils::ConvertBodyType(rigidBody.Type);
		bodydef.position = { worldTransform.Translation.x, worldTransform.Translation.y };
		bodydef.angle = worldTransform.Rotation.z;
		bodydef.fixedRotation = rigidBody.FixedRotation;
		bodydef.bullet = rigidBody.IsBullet;
		bodydef.awake = rigidBody.Awake;
		bodydef.enabled = rigidBody.Enabled;
		bodydef.gravityScale = rigidBody.GravityScale;
		bodydef.allowSleep = rigidBody.AllowSleep;
		bodydef.userData.pointer = (uintptr_t)entity.GetUUID().Value();

		rigidBody.RuntimeBody = m_PhysicsScene->GetWorld()->CreateBody(&bodydef);
	}

	void Scene::CreateRuntimeBoxCollider2D(Entity entity, const TransformComponent& worldTransform)
	{
		auto& rigidBody = entity.GetComponent<RigidBody2DComponent>();
		auto& boxCollider = entity.GetComponent<BoxCollider2DComponent>();
		SK_CORE_VERIFY(boxCollider.RuntimeCollider == nullptr);

		b2PolygonShape shape;
		shape.SetAsBox(boxCollider.Size.x * worldTransform.Scale.x, boxCollider.Size.y * worldTransform.Scale.y, { boxCollider.Offset.x, boxCollider.Offset.y }, boxCollider.Rotation);

		b2FixtureDef fixturedef;
		fixturedef.shape = &shape;
		fixturedef.friction = boxCollider.Friction;
		fixturedef.density = boxCollider.Density;
		fixturedef.restitution = boxCollider.Restitution;
		fixturedef.restitutionThreshold = boxCollider.RestitutionThreshold;
		fixturedef.isSensor = boxCollider.IsSensor;

		boxCollider.RuntimeCollider = rigidBody.RuntimeBody->CreateFixture(&fixturedef);
	}

	void Scene::CreateRuntimeCircleCollider2D(Entity entity, const TransformComponent& worldTransform)
	{
		auto& rigidBody = entity.GetComponent<RigidBody2DComponent>();
		auto& circleCollider = entity.GetComponent<CircleCollider2DComponent>();
		SK_CORE_VERIFY(circleCollider.RuntimeCollider == nullptr);

		b2CircleShape shape;
		shape.m_radius = circleCollider.Radius * worldTransform.Scale.x;
		shape.m_p = { circleCollider.Offset.x, circleCollider.Offset.y };

		b2FixtureDef fixturedef;
		fixturedef.shape = &shape;
		fixturedef.friction = circleCollider.Friction;
		fixturedef.density = circleCollider.Density;
		fixturedef.restitution = circleCollider.Restitution;
		fixturedef.restitutionThreshold = circleCollider.RestitutionThreshold;
		fixturedef.isSensor = circleCollider.IsSensor;

		circleCollider.RuntimeCollider = rigidBody.RuntimeBody->CreateFixture(&fixturedef);
	}

	void Scene::CreateRuntimeDistanceJoint2D(Entity entity)
	{
		auto& distanceJointComp = entity.GetComponent<DistanceJointComponent>();

		if (!IsValidEntityID(distanceJointComp.ConnectedEntity))
			return;

		Entity connectedEntity = GetEntityByID(distanceJointComp.ConnectedEntity);

		auto connectedBody = m_PhysicsScene->GetBody(connectedEntity);
		auto body = m_PhysicsScene->GetBody(entity);
		if (!body || !connectedBody)
			return;

		b2DistanceJointDef def;
		const b2Vec2 anchorA = body->GetPosition() + Physics2DUtils::ToB2Vec(distanceJointComp.AnchorOffsetA);
		const b2Vec2 anchorB = connectedBody->GetPosition() + Physics2DUtils::ToB2Vec(distanceJointComp.AnchorOffsetB);

		def.Initialize(body, connectedBody, anchorA, anchorB);
		def.collideConnected = distanceJointComp.CollideConnected;

		if (distanceJointComp.MinLength >= 0.0f)
			def.minLength = distanceJointComp.MinLength;

		if (distanceJointComp.MaxLength >= 0.0f)
			def.maxLength = distanceJointComp.MaxLength;

		def.stiffness = distanceJointComp.Stiffness;
		def.damping = distanceJointComp.Damping;

		auto world = m_PhysicsScene->GetWorld();
		b2Joint* joint = world->CreateJoint(&def);
		distanceJointComp.RuntimeJoint = (b2DistanceJoint*)joint;
	}

	void Scene::CreateRuntimeHingeJoint2D(Entity entity)
	{
		auto& hingeJointComp = entity.GetComponent<HingeJointComponent>();

		if (!IsValidEntityID(hingeJointComp.ConnectedEntity))
			return;

		Entity connectedEntity = GetEntityByID(hingeJointComp.ConnectedEntity);

		auto connectedBody = m_PhysicsScene->GetBody(connectedEntity);
		auto body = m_PhysicsScene->GetBody(entity);
		if (!body || !connectedBody)
			return;

		b2RevoluteJointDef def;
		def.Initialize(body, connectedBody, Physics2DUtils::ToB2Vec(hingeJointComp.Anchor));
		def.collideConnected = hingeJointComp.CollideConnected;

		def.lowerAngle = hingeJointComp.LowerAngle;
		def.upperAngle = hingeJointComp.UpperAngle;
		def.enableMotor = hingeJointComp.EnableMotor;
		def.motorSpeed = hingeJointComp.MotorSpeed;
		def.maxMotorTorque = hingeJointComp.MaxMotorTorque;

		auto world = m_PhysicsScene->GetWorld();
		b2Joint* joint = world->CreateJoint(&def);
		hingeJointComp.RuntimeJoint = (b2RevoluteJoint*)joint;
	}

	void Scene::CreateRuntimePrismaticJoint2D(Entity entity)
	{
		auto& component = entity.GetComponent<PrismaticJointComponent>();

		if (!IsValidEntityID(component.ConnectedEntity))
			return;

		Entity connectedEntity = GetEntityByID(component.ConnectedEntity);

		auto connectedBody = m_PhysicsScene->GetBody(connectedEntity);
		auto body = m_PhysicsScene->GetBody(entity);
		if (!body || !connectedBody)
			return;

		b2PrismaticJointDef def;
		def.Initialize(body, connectedBody, Physics2DUtils::ToB2Vec(component.Anchor), Physics2DUtils::ToB2Vec(component.Axis));
		def.collideConnected = component.CollideConnected;

		def.enableLimit = component.EnableLimit;
		def.lowerTranslation = component.LowerTranslation;
		def.upperTranslation = component.UpperTranslation;
		def.enableMotor = component.EnableMotor;
		def.motorSpeed = component.MotorSpeed;
		def.maxMotorForce = component.MaxMotorForce;

		auto world = m_PhysicsScene->GetWorld();
		b2Joint* joint = world->CreateJoint(&def);
		component.RuntimeJoint = (b2PrismaticJoint*)joint;
	}

	void Scene::CreateRuntimePulleyJoint2D(Entity entity)
	{
		auto& component = entity.GetComponent<PulleyJointComponent>();

		if (!IsValidEntityID(component.ConnectedEntity))
			return;

		Entity connectedEntity = GetEntityByID(component.ConnectedEntity);

		b2Body* connectedBody = m_PhysicsScene->GetBody(connectedEntity);
		auto body = m_PhysicsScene->GetBody(entity);
		if (!body || !connectedBody)
			return;

		b2PulleyJointDef def;
		def.Initialize(body, connectedBody,
					   Physics2DUtils::ToB2Vec(component.GroundAnchorA),
					   Physics2DUtils::ToB2Vec(component.GroundAnchorB),
					   body->GetPosition() + Physics2DUtils::ToB2Vec(component.AnchorA),
					   connectedBody->GetPosition() + Physics2DUtils::ToB2Vec(component.AnchorB),
					   component.Ratio);

		def.collideConnected = component.CollideConnected;

		auto world = m_PhysicsScene->GetWorld();
		b2Joint* joint = world->CreateJoint(&def);
		component.RuntimeJoint = (b2PulleyJoint*)joint;
	}

}
