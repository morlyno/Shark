#include "skpch.h"
#include "Scean.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Scean/Components.h"
#include "Shark/Scean/Entity.h"

namespace Shark {

	Scean::Scean()
	{
	}

	Scean::~Scean()
	{
	}

	void Scean::OnUpdateRuntime(TimeStep ts)
	{
		auto view = m_Registry.view<NativeScriptComponent>();
		for (auto entityID : view)
		{
			auto& nsc = view.get<NativeScriptComponent>(entityID);
			nsc.Script->OnUpdate(ts);
		}

		if (!m_Registry.valid(m_ActiveCameraID) || !m_Registry.has<CameraComponent, TransformComponent>(m_ActiveCameraID))
		{
			SK_CORE_WARN("Active Camera Entity is Invalid");
			return;
		}
		auto [camera, transform] = m_Registry.get<CameraComponent, TransformComponent>(m_ActiveCameraID);

		Renderer2D::BeginScean(camera.Camera, DirectX::XMMatrixInverse(nullptr, transform.GetTranform()));

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

	Entity Scean::CreateEntity(const std::string& tag)
	{
		Entity entity{ m_Registry.create(), this };
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
		return { m_ActiveCameraID, this };
	}

}