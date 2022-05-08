#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Scene/Scene.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Scene/Components/SpriteRendererComponent.h"
#include "Shark/Scene/Components/CameraComponent.h"
#include "Shark/Math/Math.h"
#include "Shark/UI/UI.h"

#include "Shark/Core/Input.h"

#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: m_Scene(scene)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_Scene && m_Scene->GetViewportWidth() != 0 && m_Scene->GetViewportHeight() != 0)
		{
			m_ViewportWidth = m_Scene->GetViewportWidth();
			m_ViewportHeight = m_Scene->GetViewportHeight();
			m_NeedsResize = false;
		}
		else
		{
			SK_CORE_WARN("SceneRenderer Unkown Viewport Size!");
			m_ViewportWidth = 1280;
			m_ViewportHeight = 720;
		}

		m_CommandBuffer = RenderCommandBuffer::Create();

		// Geometry
		{
			FrameBufferSpecification fbspecs;
			fbspecs.Width = m_ViewportWidth;
			fbspecs.Height = m_ViewportHeight;
			fbspecs.Atachments = { ImageFormat::RGBA8, ImageFormat::R32_SINT, ImageFormat::Depth };
			fbspecs.Atachments[1].Blend = false;
			fbspecs.ClearColor = { 0.1f, 0.1f, 0.1f, 1.0f };
			m_GeometryFrameBuffer = FrameBuffer::Create(fbspecs);
			m_Renderer2D = Ref<Renderer2D>::Create(m_GeometryFrameBuffer);
		}


		// External Composite
		{
			FrameBufferSpecification fbspecs;
			fbspecs.Width = m_ViewportWidth;
			fbspecs.Height = m_ViewportHeight;
			fbspecs.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			fbspecs.Atachments = { ImageFormat::RGBA8, ImageFormat::R32_SINT, ImageFormat::Depth };
			fbspecs.Atachments[1].Blend = false;
			fbspecs.Atachments[0].Image = m_GeometryFrameBuffer->GetImage(0);
			fbspecs.Atachments[1].Image = m_GeometryFrameBuffer->GetImage(1);
			fbspecs.Atachments[2].Image = m_GeometryFrameBuffer->GetDepthImage();
			m_ExternalCompositeFrameBuffer = FrameBuffer::Create(fbspecs);
		}

	}

	SceneRenderer::~SceneRenderer()
	{
		SK_PROFILE_FUNCTION();
	}

	void SceneRenderer::BeginScene(const glm::mat4& viewProj)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_NeedsResize && m_ViewportWidth != 0 && m_ViewportHeight != 0)
		{
			SK_PROFILE_SCOPED("SceneRenderer::BeginScene Resize");

			m_GeometryFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_ExternalCompositeFrameBuffer->Resize(m_ViewportWidth, m_ViewportHeight);
			m_NeedsResize = false;
		}

		//m_Renderer2D->SetRenderTarget(m_GeometryFrameBuffer);
		m_Renderer2D->BeginScene(viewProj);
	}

	void SceneRenderer::EndScene()
	{
		SK_PROFILE_FUNCTION();

		m_CommandBuffer->Begin();

		m_GeometryFrameBuffer->ClearAtachment(m_CommandBuffer, 0);
		m_GeometryFrameBuffer->ClearAtachment(m_CommandBuffer, 1, { -1.0f, -1.0f, -1.0f, -1.0f });
		m_GeometryFrameBuffer->ClearDepth(m_CommandBuffer);

		m_CommandBuffer->End();
		m_CommandBuffer->Execute();

		m_Renderer2D->EndScene();
	}

	void SceneRenderer::SubmitQuad(const glm::vec3& position, const glm::vec3& roation, const glm::vec3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, int id)
	{
		SK_PROFILE_FUNCTION();
		
		m_Renderer2D->DrawRotatedQuad(position, roation, scaling, texture, tilingfactor, tintcolor, id);
	}

	void SceneRenderer::SubmitCirlce(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, float thickness, float fade, int id)
	{
		SK_PROFILE_FUNCTION();
		
		m_Renderer2D->DrawFilledCircle(position, rotation, scaling, color, thickness, fade, id);
	}

	void SceneRenderer::Resize(uint32_t width, uint32_t height)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_ViewportWidth == width && m_ViewportHeight == height)
			return;

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
				UI::BeginControlsGrid();
				UI::EndControls();

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Statistics", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				UI::BeginControlsGrid();
				const auto& stats = m_Renderer2D->GetStatistics();
				UI::Control("DrawCalls", fmt::format("{}", stats.DrawCalls));
				UI::Control("Quads", fmt::format("{}", stats.QuadCount));
				UI::Control("Cirlces", fmt::format("{}", stats.CircleCount));
				UI::Control("Lines", fmt::format("{}", stats.LineCount));
				UI::Control("LinesOnTop", fmt::format("{}", stats.LineOnTopCount));
				UI::Control("Vertices", fmt::format("{}", stats.VertexCount));
				UI::Control("Indices", fmt::format("{}", stats.IndexCount));
				UI::Control("Textures", fmt::format("{}", stats.TextureCount));
				UI::EndControls();

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("GPU Times", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				UI::BeginControlsGrid();
				const auto& stats = m_Renderer2D->GetStatistics();
				UI::Control("GeometryPass", fmt::format("{:.4f}ms", stats.GeometryPassTime.MilliSeconds()));
				UI::EndControls();

				ImGui::TreePop();
			}

		}
	}

}
