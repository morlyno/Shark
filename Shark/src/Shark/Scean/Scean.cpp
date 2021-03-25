#include "skpch.h"
#include "Scean.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Scean/Entity.h"
#include "Shark/Scean/Components/Components.h"

namespace Shark {

	Scean::Scean()
	{
	}

	Scean::~Scean()
	{
	}

	void Scean::OnUpdateRuntime(TimeStep ts)
	{
		m_World.Update();

		{
			auto group = m_Registry.group<RigidBodyComponent>(entt::get<TransformComponent>);
			for (auto entityID : group)
			{
				auto& tc = group.get<TransformComponent>(entityID);
				auto& body = group.get<RigidBodyComponent>(entityID).Body;

				tc.Position.x = body.GetPosition().x;
				tc.Position.y = body.GetPosition().y;

				tc.Rotation.z = body.GetAngle();

				tc.Scaling.x = body.GetSize().x;
				tc.Scaling.y = body.GetSize().y;
			}
		}

		{
			auto view = m_Registry.view<NativeScriptComponent>();
			for (auto entityID : view)
			{
				auto& nsc = view.get<NativeScriptComponent>(entityID);
				nsc.Script->OnUpdate(ts);
			}
		}

		if (!m_Registry.valid(m_ActiveCameraID) || !m_Registry.has<CameraComponent, TransformComponent>(m_ActiveCameraID))
		{
			SK_CORE_WARN("Active Camera Entity is Invalid");
			return;
		}
		auto [camera, transform] = m_Registry.get<CameraComponent, TransformComponent>(m_ActiveCameraID);


		Renderer2D::BeginScean(camera.Camera, DirectX::XMMatrixInverse(nullptr, transform.GetTranform()));
		{
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, spriterenderer] = group.get<TransformComponent, SpriteRendererComponent>(entity);
				if (spriterenderer.Texture)
					Renderer2D::DrawQuad(transform.GetTranform(), spriterenderer.Texture, spriterenderer.TilingFactor, spriterenderer.Color);
				else
					Renderer2D::DrawQuad(transform.GetTranform(), spriterenderer.Color);
			}
		}
		Renderer2D::EndScean();
	}

	void Scean::OnUpdateEditor(TimeStep ts, EditorCamera& camera)
	{
		Renderer2D::BeginScean(camera);

		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entity : group)
		{
			auto [transform, spriterenderer] = group.get<TransformComponent, SpriteRendererComponent>(entity);
			if (spriterenderer.Texture)
				Renderer2D::DrawQuad(transform.GetTranform(), spriterenderer.Texture, spriterenderer.TilingFactor, spriterenderer.Color);
			else
				Renderer2D::DrawQuad(transform.GetTranform(), spriterenderer.Color);
		}

		Renderer2D::EndScean();
	}

	void Scean::OnSceanPlay()
	{

		m_SceanState.RigidBodyStates.reserve(m_Registry.size());
		m_SceanState.Registry.reserve(m_Registry.size());
		m_Registry.each([&](auto entityID)
			{
				entt::entity id = m_SceanState.Registry.create(entityID);
				SK_CORE_ASSERT(id == entityID, "IDs are not the same");
				if (m_Registry.has<TagComponent>(entityID))
					m_SceanState.Registry.emplace<TagComponent>(id, m_Registry.get<TagComponent>(entityID));
				if (m_Registry.has<TransformComponent>(entityID))
					m_SceanState.Registry.emplace<TransformComponent>(id, m_Registry.get<TransformComponent>(entityID));
				if (m_Registry.has<SpriteRendererComponent>(entityID))
					m_SceanState.Registry.emplace<SpriteRendererComponent>(id, m_Registry.get<SpriteRendererComponent>(entityID));
				if (m_Registry.has<CameraComponent>(entityID))
					m_SceanState.Registry.emplace<CameraComponent>(id, m_Registry.get<CameraComponent>(entityID));
				if (m_Registry.has<RigidBodyComponent>(entityID))
				{
					auto& rbc = m_Registry.get<RigidBodyComponent>(entityID);
					m_SceanState.Registry.emplace<RigidBodyComponent>(id, rbc);
					m_SceanState.RigidBodyStates[(uint32_t)entityID] = rbc.Body.GetCurrentState();
				}
			});
		m_SceanState.ActiveCameraID = m_ActiveCameraID;

		{
			auto view = m_Registry.view<NativeScriptComponent>();
			for (auto entityID : view)
			{
				auto& comp = view.get<NativeScriptComponent>(entityID);
				Entity entity{ entityID, WeakRef<Scean>::Create(this) };
				comp.CreateScript(entity);
				comp.Script->OnCreate();
			}
		}

		ResizeCameras((float)m_ViewportWidth, (float)m_ViewportHeight);

	}

	void Scean::OnSceanStop()
	{

		{
			auto view = m_Registry.view<NativeScriptComponent>();
			for (auto entityID : view)
			{
				auto& comp = view.get<NativeScriptComponent>(entityID);
				comp.DestroyScript(comp.Script);
				comp.Script->OnDestroy();
			}
		}

		m_Registry = std::move(m_SceanState.Registry);
		m_ActiveCameraID = m_SceanState.ActiveCameraID;

		{
			auto group = m_Registry.group<RigidBodyComponent>(entt::get<TransformComponent>);
			for (auto entityID : group)
			{
				auto& body = group.get<RigidBodyComponent>(entityID).Body;
				body.SetState(m_SceanState.RigidBodyStates[(uint32_t)entityID]);
			}
		}

		m_World.Flush();

	}

	Entity Scean::CreateEntity(const std::string& tag)
	{
		Entity entity{ m_Registry.create(), WeakRef<Scean>::Create(this) };
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
		return { m_ActiveCameraID, WeakRef<Scean>::Create(this) };
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
	}

	template<>
	void Scean::OnComponentAdded<RigidBodyComponent>(Entity entity, RigidBodyComponent& comp)
	{
	}

}