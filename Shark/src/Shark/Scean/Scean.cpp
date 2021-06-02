#include "skpch.h"
#include "Scean.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Render/TestRenderer2D.h"
#include "Shark/Scean/Entity.h"
#include "Shark/Scean/Components/Components.h"

namespace Shark {

	Scean::Scean()
	{
	}

	Scean::~Scean()
	{
	}

	Scean::Scean(const Scean& other)
	{
		m_World.SetGravity(other.m_World.GetGravity());
		other.m_Registry.each([this, &other](auto ID)
		{
			Entity e{ ID, Weak((Scean*)&other) };
			CreateEntity(e, true);
		});

		m_ActiveCameraID = other.m_ActiveCameraID;
		m_ViewportWidth = other.m_ViewportWidth;
		m_ViewportHeight = other.m_ViewportHeight;
	}

	Scean& Scean::operator=(const Scean& other)
	{
		m_World = World(other.m_World.GetGravity());
		m_Registry = entt::registry{};
		m_Registry.reserve(other.m_Registry.capacity());

		m_World.SetGravity(other.m_World.GetGravity());
		other.m_Registry.each([this, &other](auto ID)
		{
			Entity e{ ID, Weak((Scean*)&other) };
			CreateEntity(e, true);
		});

		m_ActiveCameraID = other.m_ActiveCameraID;
		m_ViewportWidth = other.m_ViewportWidth;
		m_ViewportHeight = other.m_ViewportHeight;

		return *this;
	}

	Scean::Scean(Scean&& other)
	{
		m_Registry = std::move(other.m_Registry);
		m_World = std::move(other.m_World);
		m_ActiveCameraID = other.m_ActiveCameraID;
		m_ViewportWidth = other.m_ViewportWidth;
		m_ViewportHeight = other.m_ViewportHeight;

		other.m_ActiveCameraID = entt::null;
		other.m_ViewportWidth = 0;
		other.m_ViewportHeight = 0;
	}

	Scean& Scean::operator=(Scean&& other)
	{
		m_Registry = std::move(other.m_Registry);
		m_World = std::move(other.m_World);
		m_ActiveCameraID = other.m_ActiveCameraID;
		m_ViewportWidth = other.m_ViewportWidth;
		m_ViewportHeight = other.m_ViewportHeight;

		other.m_ActiveCameraID = entt::null;
		other.m_ViewportWidth = 0;
		other.m_ViewportHeight = 0;
		return *this;
	}

	void Scean::OnUpdateRuntime(TimeStep ts)
	{
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

#if SK_TEST_RENDERER
		TestRenderer::BeginScean(camera.Camera, DirectX::XMMatrixInverse(nullptr, transform.GetTranform()));
		{
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entityID : group)
				TestRenderer::DrawEntity({ entityID, Weak(this) });
		}
		TestRenderer::EndScean();
#else
		Renderer2D::BeginScean(camera.Camera, DirectX::XMMatrixInverse(nullptr, transform.GetTranform()));
		{
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entityID : group)
				Renderer2D::DrawEntity({ entityID, Weak(this) });
		}
		Renderer2D::EndScean();
#endif
	}

	void Scean::OnUpdateEditor(TimeStep ts, EditorCamera& camera)
	{
#if SK_TEST_RENDERER
		TestRenderer::BeginScean(camera);
		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entityID : group)
			TestRenderer::DrawEntity({ entityID, Weak(this) });
		TestRenderer::EndScean();
#else
		Renderer2D::BeginScean(camera);

		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entityID : group)
			Renderer2D::DrawEntity({ entityID, Weak(this) });

		// TODO: Draw Selected Entity
		//if (selectedentity)
			// Draw Selcted Entity

		Renderer2D::EndScean();
#endif
	}

	void Scean::OnEventRuntime(Event& event)
	{
		auto view = m_Registry.view<NativeScriptComponent>();
		for (auto entityID : view)
		{
			auto& nsc = view.get<NativeScriptComponent>(entityID);
			if (nsc.Script)
				nsc.Script->OnEvent(event);
		}
	}

	void Scean::OnEventEditor(Event& event)
	{
	}

	void Scean::OnSceanPlay()
	{
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

	void Scean::OnSceanStop()
	{
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

	Entity Scean::CreateEntity(Entity other, bool hint)
	{
		entt::entity entityID = hint ? m_Registry.create(other) : m_Registry.create();
		Entity e{ entityID, Weak(this) };

		if (other.HasComponent<TagComponent>())
			e.AddComponent<TagComponent>(other.GetComponent<TagComponent>());

		if (other.HasComponent<TransformComponent>())
			e.AddComponent<TransformComponent>(other.GetComponent<TransformComponent>());

		if (other.HasComponent<SpriteRendererComponent>())
			e.AddComponent<SpriteRendererComponent>(other.GetComponent<SpriteRendererComponent>());

		if (other.HasComponent<CameraComponent>())
			e.AddComponent<CameraComponent>(other.GetComponent<CameraComponent>());

		if (other.HasComponent<NativeScriptComponent>())
		{
			e.AddComponent<NativeScriptComponent>(other.GetComponent<NativeScriptComponent>());
			if (m_AddEditorData && other.HasComponent<EditorData::NaticeScriptComponent>())
				e.GetComponent<EditorData::NaticeScriptComponent>() = other.GetComponent<EditorData::NaticeScriptComponent>();
		}

		if (other.HasComponent<RigidBodyComponent>())
		{
			auto& othercomp = other.GetComponent<RigidBodyComponent>();
			auto& comp = e.AddComponent<RigidBodyComponent>();
			comp.Body.SetState(othercomp.Body.GetCurrentState());
		}
		if (other.HasComponent<BoxColliderComponent>())
		{
			auto& othercomp = other.GetComponent<BoxColliderComponent>();
			auto& comp = e.AddComponent<BoxColliderComponent>();
			comp.Collider.SetState(othercomp.Collider.GetCurrentState());
		}

		return e;
	}

	Entity Scean::CreateEntity(const std::string& tag)
	{
		Entity entity{ m_Registry.create(), Weak(this) };
		auto& tagcomp = entity.AddComponent<TagComponent>();
		tagcomp.Tag = tag.empty() ? "Entity" : tag;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	void Scean::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity);
	}

	Entity Scean::GetActiveCamera()
	{
		return { m_ActiveCameraID, Weak(this) };
	}

	void Scean::SetActiveCamera(Entity cameraentity)
	{
		m_ActiveCameraID = cameraentity;
	}

	void Scean::ResizeCameras(float width, float height)
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entityID : view)
		{
			auto& cc = view.get<CameraComponent>(entityID);
			cc.Camera.Resize(width, height);
		}
	}

	bool Scean::IsValidEntity(Entity entity)
	{
		return m_Registry.valid(entity);
	}

	void Scean::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<SelectionChangedEvent>(SK_BIND_EVENT_FN(Scean::OnSelectionChanged));
	}

	bool Scean::OnSelectionChanged(SelectionChangedEvent& event)
	{
		m_SelectedEntity = event.GetSelectedEntity();
		return false;
	}



	template<typename Component>
	void Scean::OnComponentAdded(Entity entity, Component& comp)
	{
		static_assert(false);
	}
	
	template<>
	void Scean::OnComponentAdded<TagComponent>(Entity entity, TagComponent& comp)
	{
	}
	
	template<>
	void Scean::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& comp)
	{
	}
	
	template<>
	void Scean::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& comp)
	{
	}
	
	template<>
	void Scean::OnComponentAdded<CameraComponent>(Entity entity,CameraComponent& comp)
	{
		comp.Camera.Resize((float)m_ViewportWidth, (float)m_ViewportHeight);
	}

	template<>
	void Scean::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& comp)
	{
		if (m_AddEditorData)
			entity.AddComponent<EditorData::NaticeScriptComponent>();
	}

	template<>
	void Scean::OnComponentAdded<RigidBodyComponent>(Entity entity, RigidBodyComponent& comp)
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
	void Scean::OnComponentAdded<BoxColliderComponent>(Entity entity, BoxColliderComponent& comp)
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

	template<>
	void Scean::OnComponentAdded<EditorData::NaticeScriptComponent>(Entity entity, EditorData::NaticeScriptComponent& comp)
	{
		return;
	}

}