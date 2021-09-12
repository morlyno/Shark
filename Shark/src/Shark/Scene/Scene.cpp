#include "skpch.h"
#include "Scene.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/TestRenderer.h"
#include "Shark/Scene/Entity.h"
#include "Shark/Scene/Components/Components.h"
#include "Shark/Render/Renderer.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	Scene::Scene()
	{
		SK_PROFILE_FUNCTION();
	}

	Scene::~Scene()
	{
		SK_PROFILE_FUNCTION();
	}

	Scene::Scene(Scene&& other)
	{
		SK_PROFILE_FUNCTION();

		m_Registry = std::move(other.m_Registry);
		m_World = std::move(other.m_World);
		m_ActiveCameraID = other.m_ActiveCameraID;
		m_ViewportWidth = other.m_ViewportWidth;
		m_ViewportHeight = other.m_ViewportHeight;
		m_FilePath = std::move(other.m_FilePath);

		other.m_ActiveCameraID = entt::null;
		other.m_ViewportWidth = 0;
		other.m_ViewportHeight = 0;
	}

	Scene& Scene::operator=(Scene&& other)
	{
		SK_PROFILE_FUNCTION();

		m_Registry = std::move(other.m_Registry);
		m_World = std::move(other.m_World);
		m_ActiveCameraID = other.m_ActiveCameraID;
		m_ViewportWidth = other.m_ViewportWidth;
		m_ViewportHeight = other.m_ViewportHeight;
		m_FilePath = std::move(other.m_FilePath);

		other.m_ActiveCameraID = entt::null;
		other.m_ViewportWidth = 0;
		other.m_ViewportHeight = 0;
		return *this;
	}

	Ref<Scene> Scene::GetCopy()
	{
		SK_PROFILE_FUNCTION();

		auto scene = Ref<Scene>::Create();
		CopyInto(scene);
		return scene;
	}

	void Scene::CopyInto(Ref<Scene> dest)
	{
		SK_PROFILE_FUNCTION();

		dest->m_World = World(m_World.GetGravity());
		dest->m_Registry = entt::registry{};
		dest->m_Registry.reserve(m_Registry.capacity());
		m_Registry.each([this, dest](auto entity)
		{
			dest->CopyEntity({ entity, this });
		});

		dest->m_ActiveCameraID = m_ActiveCameraID;
		dest->m_ViewportWidth = m_ViewportWidth;
		dest->m_ViewportHeight = m_ViewportHeight;
		dest->m_FilePath = m_FilePath;
	}

	void Scene::OnUpdateRuntime(TimeStep ts)
	{
		SK_PROFILE_FUNCTION();

		m_World.Update(ts);

		{
			{
				auto group = m_Registry.group<RigidBodyComponent>(entt::get<TransformComponent>);
				for (auto entityID : group)
				{
					auto& tc = group.get<TransformComponent>(entityID);
					auto& body = group.get<RigidBodyComponent>(entityID).Body;

					tc.Position.x = body.GetPosition().x;
					tc.Position.y = body.GetPosition().y;

					tc.Rotation.z = body.GetAngle();
				}
			}

			{
				auto view = m_Registry.view<NativeScriptComponent>();
				for (auto entityID : view)
				{
					auto& nsc = view.get<NativeScriptComponent>(entityID);
					if (nsc.Script)
						nsc.Script->OnUpdate(ts);
				}
			}
		}

		if (!m_Registry.valid(m_ActiveCameraID) || !m_Registry.has<CameraComponent, TransformComponent>(m_ActiveCameraID))
		{
			SK_CORE_WARN("Active Camera Entity is Invalid");
			return;
		}
		auto [camera, transform] = m_Registry.get<CameraComponent, TransformComponent>(m_ActiveCameraID);

		{
			SK_PROFILE_SCOPE("Render Scene Runtime");

			Renderer2D::BeginScene(camera.Camera, DirectX::XMMatrixInverse(nullptr, transform.GetTranform()));
			{
				auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
				for (auto entityID : group)
					Renderer2D::DrawEntity({ entityID, this });
			}
			Renderer2D::EndScene();
		}
	}

	void Scene::OnUpdateEditor(TimeStep ts, EditorCamera& camera)
	{
		SK_PROFILE_FUNCTION();
		{
			SK_PROFILE_SCOPE("Render Scene Editor");

			Renderer2D::BeginScene(camera);
			{
				auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
				for (auto entityID : group)
					Renderer2D::DrawEntity({ entityID, this });
			}
			Renderer2D::EndScene();
		}
	}

	void Scene::OnEventRuntime(Event& event)
	{
		auto view = m_Registry.view<NativeScriptComponent>();
		for (auto entityID : view)
		{
			auto& nsc = view.get<NativeScriptComponent>(entityID);
			if (nsc.Script)
				nsc.Script->OnEvent(event);
		}
	}

	void Scene::OnEventEditor(Event& event)
	{
	}

	void Scene::OnScenePlay()
	{
		SK_PROFILE_FUNCTION();

		m_World.Flush();

		auto view = m_Registry.view<NativeScriptComponent>();
		for (auto entityID : view)
		{
			auto& comp = view.get<NativeScriptComponent>(entityID);
			Entity entity{ entityID, Weak(this) };
			if (comp.CreateScript)
			{
				comp.Script = comp.CreateScript(entity);
				comp.Script->OnCreate();
			}
		}

		ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	void Scene::OnSceneStop()
	{
		SK_PROFILE_FUNCTION();

		auto view = m_Registry.view<NativeScriptComponent>();
		for (auto entityID : view)
		{
			auto& comp = view.get<NativeScriptComponent>(entityID);
			if (comp.Script)
			{
				comp.Script->OnDestroy();
				comp.DestroyScript(comp.Script);
			}
		}

		m_World.Flush();
	}

	Entity Scene::CopyEntity(Entity srcEntity)
	{
		SK_PROFILE_FUNCTION();

		Entity newEntity = CreateEntity(srcEntity);

		CopyComponentIfExists<TagComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<TransformComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<SpriteRendererComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<CameraComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<NativeScriptComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<RigidBodyComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);
		CopyComponentIfExists<BoxColliderComponent>(srcEntity.m_EntityHandle, srcEntity.m_Scene->m_Registry, newEntity, m_Registry);

		return newEntity;
	}

	Entity Scene::CreateEntity(const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		Entity entity{ m_Registry.create(), this };
		auto& tagcomp = entity.AddComponent<TagComponent>();
		tagcomp.Tag = tag.empty() ? "new Entity" : tag;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	Entity Scene::CreateEntity(entt::entity hint, const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		Entity entity{ m_Registry.create(hint), this };
		auto& tagcomp = entity.AddComponent<TagComponent>();
		tagcomp.Tag = tag.empty() ? "new Entity" : tag;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		SK_PROFILE_FUNCTION();

		m_Registry.destroy(entity);
	}

	bool Scene::IsValidEntity(Entity entity)const
	{
		SK_PROFILE_FUNCTION();

		return m_Registry.valid(entity);
	}

	Entity Scene::GetActiveCamera()
	{
		return { m_ActiveCameraID, Weak(this) };
	}

	void Scene::SetActiveCamera(Entity cameraentity)
	{
		m_ActiveCameraID = cameraentity;
	}

	void Scene::ResizeCameras(float width, float height)
	{
		SK_PROFILE_FUNCTION();

		auto view = m_Registry.view<CameraComponent>();
		for (auto entityID : view)
		{
			auto& cc = view.get<CameraComponent>(entityID);
			cc.Camera.Resize(width, height);
		}
	}




	template<typename Component>
	void Scene::OnComponentAdded(Entity entity, Component& comp)
	{
		static_assert(false);
	}
	
	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& comp)
	{
	}
	
	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& comp)
	{
	}
	
	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& comp)
	{
	}
	
	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity,CameraComponent& comp)
	{
		comp.Camera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& comp)
	{
	}

	template<>
	void Scene::OnComponentAdded<RigidBodyComponent>(Entity entity, RigidBodyComponent& comp)
	{
		if (!entity.HasComponent<TransformComponent>())
			entity.AddComponent<TransformComponent>();

		comp.Body = m_World.CreateRigidBody();
		BodyUserData* userdata = new BodyUserData();
		userdata->Entity = entity;
		userdata->RigidBody = &comp.Body;
		comp.Body.SetUserData(userdata);

		auto& tc = entity.GetComponent<TransformComponent>();
		comp.Body.SetTransform(tc.Position.x, tc.Position.y, tc.Rotation.z);
	}

	template<>
	void Scene::OnComponentAdded<BoxColliderComponent>(Entity entity, BoxColliderComponent& comp)
	{
		if (!entity.HasComponent<TransformComponent>())
			entity.AddComponent<TransformComponent>();
		if (!entity.HasComponent<RigidBodyComponent>())
			entity.AddComponent<RigidBodyComponent>();

		auto& rb = entity.GetComponent<RigidBodyComponent>();
		comp.Collider = rb.Body.CreateBoxCollider();

		auto& tc = entity.GetComponent<TransformComponent>();
		comp.Collider.Resize(tc.Scaling.x, tc.Scaling.y);
	}

}
