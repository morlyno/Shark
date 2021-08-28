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
			Entity e{ entity, this };
			dest->CopyEntity(e, true);
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

	Entity Scene::CopyEntity(Entity other, bool hint)
	{
		SK_PROFILE_FUNCTION();

		entt::entity entityID = hint ? m_Registry.create(other) : m_Registry.create();
		auto e = Entity{ entityID, this };

		TryCopyComponent<TagComponent>(other, e);
		TryCopyComponent<TransformComponent>(other, e);
		TryCopyComponent<SpriteRendererComponent>(other, e);
		TryCopyComponent<CameraComponent>(other, e);
		TryCopyComponent<NativeScriptComponent>(other, e);
		TryCopyComponent<RigidBodyComponent>(other, e);
		TryCopyComponent<BoxColliderComponent>(other, e);

		return e;
	}

	Entity Scene::CreateEntity(const std::string& tag)
	{
		SK_PROFILE_FUNCTION();

		Entity entity{ m_Registry.create(), Weak(this) };
		auto& tagcomp = entity.AddComponent<TagComponent>();
		tagcomp.Tag = tag.empty() ? "Entity" : tag;
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


	template<typename Comp>
	void Scene::CopyComponent(Entity src, Entity dest)
	{
		static_assert(false);
	}

	template<>
	void Scene::CopyComponent<TagComponent>(Entity src, Entity dest)
	{
		SK_CORE_ASSERT(IsValidEntity(src));
		SK_CORE_ASSERT(src.HasComponent<TagComponent>());
		TagComponent& destComp = dest.TryAddComponent<TagComponent>();
		TagComponent& srcComp = src.GetComponent<TagComponent>();
		destComp.Tag = srcComp.Tag;
	}

	template<>
	void Scene::CopyComponent<TransformComponent>(Entity src, Entity dest)
	{
		SK_CORE_ASSERT(IsValidEntity(src));
		SK_CORE_ASSERT(src.HasComponent<TransformComponent>());
		auto& destComp = dest.TryAddComponent<TransformComponent>();
		auto& srcComp = src.GetComponent<TransformComponent>();
		destComp.Position = srcComp.Position;
		destComp.Rotation = srcComp.Rotation;
		destComp.Scaling = srcComp.Scaling;
	}

	template<>
	void Scene::CopyComponent<SpriteRendererComponent>(Entity src, Entity dest)
	{
		SK_CORE_ASSERT(IsValidEntity(src));
		SK_CORE_ASSERT(src.HasComponent<SpriteRendererComponent>());
		auto& destComp = dest.TryAddComponent<SpriteRendererComponent>();
		auto& srcComp = src.GetComponent<SpriteRendererComponent>();
		destComp.Color = srcComp.Color;
		destComp.Texture = srcComp.Texture;
		destComp.TilingFactor = srcComp.TilingFactor;
		destComp.Geometry = srcComp.Geometry;
	}

	template<>
	void Scene::CopyComponent<CameraComponent>(Entity src, Entity dest)
	{
		SK_CORE_ASSERT(IsValidEntity(src));
		SK_CORE_ASSERT(src.HasComponent<CameraComponent>());
		auto& destComp = dest.TryAddComponent<CameraComponent>();
		auto& srcComp = src.GetComponent<CameraComponent>();
		destComp.Camera = srcComp.Camera;
	}

	template<>
	void Scene::CopyComponent<NativeScriptComponent>(Entity src, Entity dest)
	{
		SK_CORE_ASSERT(IsValidEntity(src));
		SK_CORE_ASSERT(src.HasComponent<NativeScriptComponent>());
		auto& destComp = dest.TryAddComponent<NativeScriptComponent>();
		auto& srcComp = src.GetComponent<NativeScriptComponent>();
		destComp.ScriptTypeName = srcComp.ScriptTypeName;
		destComp.CreateScript = srcComp.CreateScript;
		destComp.DestroyScript = srcComp.DestroyScript;
	}
	
	template<>
	void Scene::CopyComponent<RigidBodyComponent>(Entity src, Entity dest)
	{
		SK_CORE_ASSERT(IsValidEntity(src));
		SK_CORE_ASSERT(src.HasComponent<RigidBodyComponent>());
		auto& destComp = dest.TryAddComponent<RigidBodyComponent>();
		auto& srcComp = src.GetComponent<RigidBodyComponent>();
		destComp.Body.SetState(srcComp.Body.GetCurrentState());
	}
	
	template<>
	void Scene::CopyComponent<BoxColliderComponent>(Entity src, Entity dest)
	{
		SK_CORE_ASSERT(IsValidEntity(src));
		SK_CORE_ASSERT(src.HasComponent<BoxColliderComponent>());
		auto& destComp = dest.TryAddComponent<BoxColliderComponent>();
		auto& srcComp = src.GetComponent<BoxColliderComponent>();
		destComp.Collider.SetState(srcComp.Collider.GetCurrentState());
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
