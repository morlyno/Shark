#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Render/Renderer2D.h"
#include "Shark/Scene/Components/SpriteRendererComponent.h"
#include "Shark/Scene/Components/CameraComponent.h"
#include "Shark/Utility/Math.h"

namespace Shark {

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: m_Scene(scene)
	{
		FrameBufferSpecification fbspecs;
		fbspecs.Width = m_Scene->GetViewportWidth();
		fbspecs.Height = m_Scene->GetViewportHeight();
		fbspecs.Atachments = { { ImageFormat::RGBA8, true }, ImageFormat::R32_SINT, ImageFormat::Depth32 };
		fbspecs.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		m_FrameBuffer = FrameBuffer::Create(fbspecs);
	}

	SceneRenderer::~SceneRenderer()
	{
	}

	void SceneRenderer::OnRender(EditorCamera& camera)
	{
		m_FrameBuffer->ClearDepth();
		m_FrameBuffer->ClearAtachment(0);
		m_FrameBuffer->ClearAtachment(1, { -1.0f, -1.0f, -1.0f, -1.0f });
		m_FrameBuffer->Bind();

		Renderer2D::BeginScene(camera);

		auto entitys = m_Scene->GetAllEntitysWith<SpriteRendererComponent>();
		for (auto& entity : entitys)
			Renderer2D::DrawEntity(entity);

		Renderer2D::EndScene();

		m_FrameBuffer->UnBind();
	}

	void SceneRenderer::OnRender()
	{
		m_FrameBuffer->ClearDepth();
		m_FrameBuffer->ClearAtachment(0);
		m_FrameBuffer->ClearAtachment(1, { -1.0f, -1.0f, -1.0f, -1.0f });
		m_FrameBuffer->Bind();

		Entity cameraEntity = m_Scene->GetActiveCamera();
		auto& camera = cameraEntity.GetComponent<CameraComponent>().Camera;
		const auto& tf = cameraEntity.GetTransform();
		auto entitys = m_Scene->GetAllEntitysWith<SpriteRendererComponent>();

		Renderer2D::BeginScene(camera, DirectX::XMMatrixInverse(nullptr, tf.GetTranform()));

		for (auto& entity : entitys)
			Renderer2D::DrawEntity(entity);

		Renderer2D::EndScene();

		m_FrameBuffer->UnBind();
	}

	void SceneRenderer::Resize(uint32_t width, uint32_t height)
	{
		m_FrameBuffer->Resize(width, height);
	}

}
