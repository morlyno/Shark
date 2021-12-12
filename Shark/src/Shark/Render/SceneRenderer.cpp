#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Scene/Scene.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Scene/Components/SpriteRendererComponent.h"
#include "Shark/Scene/Components/CameraComponent.h"
#include "Shark/Utility/Math.h"
#include "Shark/Utility/UI.h"

#include "Shark/Core/Input.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	SceneRenderer::SceneRenderer(Ref<Scene> scene, const SceneRendererOptions& options)
		: m_Scene(scene), m_Options(options)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_Scene->GetViewportWidth() != 0 && m_Scene->GetViewportHeight() != 0)
		{
			m_ViewportWidth = m_Scene->GetViewportWidth();
			m_ViewportHeight = m_Scene->GetViewportHeight();
			m_NeedsResize = false;
		}
		else
		{
			SK_CORE_WARN("SceneRenderer Invalid Viewport Size!");
			m_ViewportWidth = 1280;
			m_ViewportHeight = 720;
		}

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
		SK_PROFILE_FUNCTION();
	}

	void SceneRenderer::BeginScene(const DirectX::XMMATRIX& viewProj)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_NeedsResize && m_ViewportWidth != 0 && m_ViewportHeight != 0)
		{
			SK_PROFILE_SCOPED("SceneRenderer::BeginScene Resize");

			m_GeometryFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_FinalFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_NeedsResize = false;
		}

		m_Renderer2D->SetRenderTarget(m_GeometryFrameBuffer);
		m_Renderer2D->BeginScene(viewProj);
	}

	void SceneRenderer::EndScene()
	{
		SK_PROFILE_FUNCTION();
		
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
		SK_PROFILE_FUNCTION();
		
		m_Renderer2D->DrawRotatedQuad(position, roation, scaling, texture, tilingfactor, tintcolor, id);
	}

	void SceneRenderer::SubmitCirlce(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& rotation, const DirectX::XMFLOAT3& scaling, const DirectX::XMFLOAT4& color, float thickness, float fade, int id)
	{
		SK_PROFILE_FUNCTION();
		
		m_Renderer2D->DrawFilledCircle(position, rotation, scaling, color, thickness, fade, id);
	}

	void SceneRenderer::SubmitColliderBox(const DirectX::XMFLOAT2& pos, float rotation, const DirectX::XMFLOAT2& scale)
	{
		SK_PROFILE_FUNCTION();
		
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
		SK_PROFILE_FUNCTION();
		
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
		SK_PROFILE_FUNCTION();
		
		m_ViewportWidth = width;
		m_ViewportHeight = height;
		m_NeedsResize = true;
	}

	void SceneRenderer::OnImGuiRender()
	{
		if (ImGui::CollapsingHeader("SceneRenderer"))
		{
			if (ImGui::TreeNodeEx("Settings", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				UI::BeginPropertyGrid();

				UI::Checkbox("Show Colliders", m_Options.ShowColliders);
				UI::Checkbox("Show Colliders On Top", m_Options.ShowCollidersOnTop);

				UI::EndProperty();

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Statistics", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				UI::BeginPropertyGrid();
				const auto& stats = m_Renderer2D->GetStatistics();
				UI::Property("DrawCalls", fmt::format("{}", stats.DrawCalls));
				UI::Property("Quads", fmt::format("{}", stats.QuadCount));
				UI::Property("Cirlces", fmt::format("{}", stats.CircleCount));
				UI::Property("Lines", fmt::format("{}", stats.LineCount));
				UI::Property("LinesOnTop", fmt::format("{}", stats.LineOnTopCount));
				UI::Property("Vertices", fmt::format("{}", stats.VertexCount));
				UI::Property("Indices", fmt::format("{}", stats.IndexCount));
				UI::Property("Textures", fmt::format("{}", stats.TextureCount));
				UI::EndProperty();

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("GPU Times", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				UI::BeginPropertyGrid();
				const auto& stats = m_Renderer2D->GetStatistics();
				UI::Property("GeometryPass", fmt::format("{:.4f}ms", stats.GeometryPassTime.MilliSeconds()));
				UI::EndProperty();

				ImGui::TreePop();
			}

		}
	}

}
