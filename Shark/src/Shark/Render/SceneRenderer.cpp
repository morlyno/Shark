#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Scene/Components/SpriteRendererComponent.h"
#include "Shark/Scene/Components/CameraComponent.h"
#include "Shark/Utility/Math.h"

#include "Shark/Core/Timer.h"

#include <random>

namespace Shark {

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: m_Scene(scene)
	{
		FrameBufferSpecification fbspecs;
		fbspecs.Width = m_Scene->GetViewportWidth();
		fbspecs.Height = m_Scene->GetViewportHeight();
		fbspecs.Atachments = { ImageFormat::RGBA8, ImageFormat::R32_SINT, ImageFormat::Depth32 };
		fbspecs.Atachments[0].Blend = true;
		fbspecs.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		m_FrameBuffer = FrameBuffer::Create(fbspecs);

		m_Renderer2D = Ref<Renderer2D>::Create(m_FrameBuffer);
	}

	SceneRenderer::~SceneRenderer()
	{
	}

	void SceneRenderer::OnRender(EditorCamera& camera)
	{
		m_FrameBuffer->ClearDepth();
		m_FrameBuffer->ClearAtachment(0);
		m_FrameBuffer->ClearAtachment(1, { -1.0f, -1.0f, -1.0f, -1.0f });

		m_Renderer2D->BeginScene(camera.GetViewProjection());

		auto entitys = m_Scene->GetAllEntitysWith<SpriteRendererComponent>();
		for (auto& e : entitys)
		{
			Entity entity{ e, m_Scene };
			auto& sr = entity.GetComponent<SpriteRendererComponent>();
			auto& tf = entity.GetTransform();

			if (sr.Geometry == SpriteRendererComponent::GeometryType::Quad)
				m_Renderer2D->DrawRotatedQuad(tf.Position, tf.Rotation, tf.Scaling, sr.Texture, sr.TilingFactor, sr.Color, (int)(uint32_t)e);
			else
				m_Renderer2D->DrawFilledCircle(tf.Position, tf.Rotation, tf.Scaling, sr.Thickness, sr.Color, (int)(uint32_t)e);
		}

		m_Renderer2D->EndScene();

		m_FrameBuffer->UnBind();

	}

	void SceneRenderer::OnRender()
	{
		m_FrameBuffer->ClearDepth();
		m_FrameBuffer->ClearAtachment(0);
		m_FrameBuffer->ClearAtachment(1, { -1.0f, -1.0f, -1.0f, -1.0f });

		Entity cameraEntity = m_Scene->GetActiveCamera();
		auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
		const auto& tf = cameraEntity.GetTransform();

		auto viewProj = DirectX::XMMatrixInverse(nullptr, tf.GetTranform()) * camera.GetProjection();

		m_Renderer2D->BeginScene(viewProj);

		auto entitys = m_Scene->GetAllEntitysWith<SpriteRendererComponent>();
		for (auto& e : entitys)
		{
			Entity entity{ e, m_Scene };
			auto& sr = entity.GetComponent<SpriteRendererComponent>();
			auto& tf = entity.GetTransform();

			if (sr.Geometry == SpriteRendererComponent::GeometryType::Quad)
				m_Renderer2D->DrawRotatedQuad(tf.Position, tf.Rotation, tf.Scaling, sr.Texture, sr.TilingFactor, sr.Color, (int)(uint32_t)e);
			else
				m_Renderer2D->DrawFilledCircle(tf.Position, tf.Rotation, tf.Scaling, sr.Thickness, sr.Color, (int)(uint32_t)e);
		}

		m_Renderer2D->EndScene();

		m_FrameBuffer->UnBind();
	}

	void SceneRenderer::Resize(uint32_t width, uint32_t height)
	{
		m_FrameBuffer->Resize(width, height);
	}

}
