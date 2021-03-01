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
		Camera* mainCamera = nullptr;
		DirectX::XMMATRIX cameraTransform;

		auto view = m_Registry.view<CameraComponent, TransformComponent>();
		for (auto entity : view)
		{
			auto [camera, transform] = view.get<CameraComponent, TransformComponent>(entity);

			if (camera.Primary)
			{
				mainCamera = &camera.Camera;
				cameraTransform = DirectX::XMMatrixInverse(nullptr, transform.GetTranform());
				break;
			}
		}

		if (!mainCamera)
		{
			SK_CORE_WARN("No Scean Camera found");
			return;
		}

		Renderer2D::BeginScean(*mainCamera, cameraTransform);

		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entity : group)
		{
			auto [transform, color] = group.get<TransformComponent, SpriteRendererComponent>(entity);
			Renderer2D::DrawQuad(transform.GetTranform(), color.Color);
		}

		Renderer2D::EndScean();
	}

	void Scean::OnUpdateEditor(TimeStep ts, EditorCamera& camera)
	{
		Renderer2D::BeginScean(camera);

		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		for (auto entity : group)
		{
			auto [transform, color] = group.get<TransformComponent, SpriteRendererComponent>(entity);

			Renderer2D::DrawQuad(transform.GetTranform(), color.Color);
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

}