#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Scene/Scene.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Scene/Components/SpriteRendererComponent.h"
#include "Shark/Scene/Components/CameraComponent.h"
#include "Shark/Utility/Math.h"

namespace Shark {

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: m_Scene(scene)
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
		m_Renderer2DFrameBuffer = FrameBuffer::Create(fbspecs);
		m_Renderer2DFrameBuffer->GetImage(0)->CreateView();
		m_Renderer2DFrameBuffer->GetImage(1)->CreateView();
		m_Renderer2DFrameBuffer->GetDepthImage()->CreateView();
		m_Renderer2D = Ref<Renderer2D>::Create(m_Renderer2DFrameBuffer);


		// Compositing
		{
			PipelineSpecification compositingPipelineSpecs;
			compositingPipelineSpecs.Shader = Renderer::GetShaderLib()->Get("FullScreen");
			compositingPipelineSpecs.DebugName = "Compositing";
			compositingPipelineSpecs.BackFaceCulling = true;
			compositingPipelineSpecs.WriteDepth = false;

			FrameBufferSpecification compositingFrameBufferSpecs;
			compositingFrameBufferSpecs.Width = m_ViewportWidth;
			compositingFrameBufferSpecs.Height = m_ViewportHeight;
			compositingFrameBufferSpecs.Atachments = { ImageFormat::RGBA8 };
			compositingFrameBufferSpecs.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			compositingPipelineSpecs.TargetFrameBuffer = FrameBuffer::Create(compositingFrameBufferSpecs);
			compositingPipelineSpecs.TargetFrameBuffer->GetImage(0)->CreateView();

			m_CompositPipeline = Pipeline::Create(compositingPipelineSpecs);
		}

	}

	SceneRenderer::~SceneRenderer()
	{
	}

	void SceneRenderer::BeginScene(const DirectX::XMMATRIX& viewProj)
	{
		if (m_NeedsResize)
		{
			m_Renderer2DFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_CompositPipeline->GetSpecification().TargetFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_NeedsResize = false;
		}

		m_CompositPipeline->GetSpecification().TargetFrameBuffer->Clear(m_CommandBuffer);

		m_Renderer2D->BeginScene(viewProj);
	}

	void SceneRenderer::EndScene()
	{
		m_Renderer2D->EndScene();

		m_CommandBuffer->Begin();

		// Composit
		Ref<Image2D> image = m_Renderer2DFrameBuffer->GetImage();
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_CompositPipeline, image);

		m_CommandBuffer->End();
		m_CommandBuffer->Execute();
	}

	void SceneRenderer::SubmitQuad(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& roation, const DirectX::XMFLOAT3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const DirectX::XMFLOAT4& tintcolor, int id)
	{
		m_Renderer2D->DrawRotatedQuad(position, roation, scaling, texture, tilingfactor, tintcolor, id);
	}

	void SceneRenderer::SubmitCirlce(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, float thickness, const DirectX::XMFLOAT4& color, int id)
	{
		m_Renderer2D->DrawFilledCircle(position, rotation, scaling, thickness, color, id);
	}

	void SceneRenderer::Resize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
		m_NeedsResize = true;
	}

}
