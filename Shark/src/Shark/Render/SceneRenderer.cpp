#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Scene/Scene.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Scene/Components/SpriteRendererComponent.h"
#include "Shark/Scene/Components/CameraComponent.h"
#include "Shark/Utility/Math.h"

namespace Shark {

	SceneRenderer::SceneRenderer(Ref<Scene> scene, const SceneRendererOptions& options)
		: m_Scene(scene), m_Options(options)
	{
		m_ViewportWidth = m_Scene->GetViewportWidth();
		m_ViewportHeight = m_Scene->GetViewportHeight();

		m_CommandBuffer = RenderCommandBuffer::Create();

		FrameBufferSpecification fbspecs;
		fbspecs.Width = m_ViewportWidth;
		fbspecs.Height = m_ViewportHeight;
		fbspecs.Atachments = { ImageFormat::RGBA8, ImageFormat::R32_SINT, ImageFormat::Depth32 };
		fbspecs.Atachments[0].Blend = true;
		fbspecs.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
		m_GeometryFrameBuffer = FrameBuffer::Create(fbspecs);
		m_Renderer2D = Ref<Renderer2D>::Create(m_GeometryFrameBuffer);

		// Final FrameBuffer
		{
			FrameBufferSpecification finalFrameBufferSpecs;
			finalFrameBufferSpecs.Width = m_ViewportWidth;
			finalFrameBufferSpecs.Height = m_ViewportHeight;
			finalFrameBufferSpecs.Atachments = { ImageFormat::RGBA8, ImageFormat::Depth32 };
			finalFrameBufferSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

			finalFrameBufferSpecs.Atachments[0].Image = m_GeometryFrameBuffer->GetImage();
			finalFrameBufferSpecs.Atachments[1].Image = m_GeometryFrameBuffer->GetDepthImage();

			m_FinalFrameBuffer = FrameBuffer::Create(finalFrameBufferSpecs);
		}

		// Create View of Final Image to Renderer Externaly
		m_FinalFrameBuffer->GetImage()->CreateView();

	}

	SceneRenderer::~SceneRenderer()
	{
	}

	void SceneRenderer::BeginScene(const DirectX::XMMATRIX& viewProj)
	{
		if (m_NeedsResize)
		{
			m_GeometryFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_FinalFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_NeedsResize = false;
		}

		m_Renderer2D->SetRenderTarget(m_GeometryFrameBuffer);
		m_Renderer2D->BeginScene(viewProj);
	}

	void SceneRenderer::EndScene()
	{
		m_CommandBuffer->Begin();

		//m_FinalFrameBuffer->Clear(m_CommandBuffer);
		m_GeometryFrameBuffer->ClearAtachment(m_CommandBuffer, 0);
		m_GeometryFrameBuffer->ClearAtachment(m_CommandBuffer, 1, { -1.0f, -1.0f, -1.0f, -1.0f });
		m_GeometryFrameBuffer->ClearDepth(m_CommandBuffer);
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();

		m_Renderer2D->EndScene();

	}

	void SceneRenderer::SubmitQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& roation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		m_Renderer2D->DrawRotatedQuad(position, roation, scaling, texture, tilingfactor, tintcolor, id);
	}

	void SceneRenderer::SubmitCirlce(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, float thickness, float fade, int id)
	{
		m_Renderer2D->DrawFilledCircle(position, rotation, scaling, color, thickness, fade, id);
	}

	void SceneRenderer::SubmitColliderBox(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scale)
	{
		if (m_Options.ShowColliders)
		{
			if (m_Options.ShowCollidersOnTop)
				m_Renderer2D->DrawRectOnTop(pos, rotation, scale, { 0.0f, 0.0f, 1.0f, 1.0f });
			else
				m_Renderer2D->DrawRect(pos, rotation, scale, { 0.0f, 0.0f, 1.0f, 1.0f });
		}
	}

	void SceneRenderer::SubmitColliderCirlce(const DirectX::XMFLOAT2& pos, float radius)
	{
		if (m_Options.ShowColliders)
		{
			if (m_Options.ShowCollidersOnTop)
				m_Renderer2D->DrawCircleOnTop(pos, radius, { 0.0f, 0.0f, 1.0f, 1.0f });
			else
				m_Renderer2D->DrawCircle(pos, radius, { 0.0f, 0.0f, 1.0f, 1.0f });
		}
	}

	void SceneRenderer::Resize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
		m_NeedsResize = true;
	}

}
