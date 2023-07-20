#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Scene/Scene.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Scene/Components.h"
#include "Shark/Math/Math.h"
#include "Shark/UI/UI.h"

#include "Shark/Input/Input.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	SceneRenderer::SceneRenderer(Ref<Scene> scene, const std::string& debugName)
		: m_Scene(scene), m_DebugName(debugName)
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
			SK_CORE_WARN_TAG("SceneRenderer", "Unkown Viewport Size");
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
			fbspecs.Atachments[1].BlendEnabled = false;
			fbspecs.ClearColor = m_ClearColor;
			fbspecs.IndipendendClearColor[1] = { -1.0f, -1.0f, -1.0f, -1.0f };
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
			fbspecs.Atachments[1].BlendEnabled = false;
			fbspecs.Atachments[0].Image = m_GeometryFrameBuffer->GetImage(0);
			fbspecs.Atachments[1].Image = m_GeometryFrameBuffer->GetImage(1);
			fbspecs.Atachments[2].Image = m_GeometryFrameBuffer->GetDepthImage();
			m_ExternalCompositeFrameBuffer = FrameBuffer::Create(fbspecs);
		}

		// Mesh Pipeline
		{
			PipelineSpecification specification;
			specification.TargetFrameBuffer = m_GeometryFrameBuffer;
			specification.Shader = Renderer::GetShaderLib()->Get("DefaultMeshShader");
			specification.Layout = VertexLayout{
				{ VertexDataType::Float3, "Position" }
			};
			specification.DebugName = "DefaultMeshShader";
			m_MeshPipeline = Pipeline::Create(specification);
		}

		m_CameraCB = ConstantBuffer::Create(sizeof(CBCamera), 0);
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
			m_Renderer2D->Resize(m_ViewportWidth, m_ViewportHeight);
			m_NeedsResize = false;
		}

		CBCamera camera;
		camera.ViewProj = viewProj;
		m_CameraCB->UploadData(Buffer::FromValue(camera));

		m_CommandBuffer->Begin();
		m_GeometryFrameBuffer->Clear(m_CommandBuffer);

		m_Renderer2D->BeginScene(viewProj);
	}

	void SceneRenderer::EndScene()
	{
		SK_PROFILE_FUNCTION();

		m_CommandBuffer->End();
		m_CommandBuffer->Execute();

		m_Renderer2D->EndScene();
	}

	void SceneRenderer::SubmitQuad(const glm::vec3& position, const glm::vec3& roation, const glm::vec3& scaling, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, int id)
	{
		m_Renderer2D->DrawRotatedQuad(position, roation, scaling, texture, tilingfactor, tintcolor, id);
	}

	void SceneRenderer::SubmitFilledCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, float thickness, float fade, int id)
	{
		m_Renderer2D->DrawFilledCircle(position, rotation, scaling, color, thickness, fade, id);
	}

	void SceneRenderer::SubmitCircle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scaling, const glm::vec4& color, int id)
	{
		m_Renderer2D->DrawCircle(position, rotation, scaling, color, id);
	}

	void SceneRenderer::SubmitCircle(const glm::mat4& transform, const glm::vec4& tintcolor, int id)
	{
		m_Renderer2D->DrawCircle(transform, tintcolor, id);
	}

	void SceneRenderer::SubmitText(const glm::mat4& transform, Ref<Font> font, const std::string& text, float kerning, float lineSpacing, const glm::vec4& color, int id)
	{
		if (!font)
			return;

		m_Renderer2D->DrawString(text, font, transform, kerning, lineSpacing, color, id);
	}

	void SceneRenderer::RenderMeshNode(const glm::mat4& parentTransform, Ref<Mesh> mesh, const Mesh::Node& node, int id)
	{
		glm::mat4 transform = parentTransform * node.Transform;

		if (node.HasMesh)
		{
			const auto& submeshes = mesh->GetSubmeshes();
			const auto& submesh = submeshes[node.MeshIndex];

			Ref<MaterialTable> materialTable = mesh->GetMaterialTable();
			if (materialTable->HasMaterial(submesh.MaterialIndex))
			{
				Ref<Material> material = materialTable->GetMaterial(submesh.MaterialIndex);
				if (material->IsValid())
				{
					material->SetMat4("c_MeshData.Transform", transform);
					material->SetInt("c_MeshData.ID", id);
				}
			}

			Renderer::RenderSubmesh(m_CommandBuffer, mesh, node.MeshIndex, m_MeshPipeline, m_CameraCB);
		}

		for (const auto& child : node.Children)
			RenderMeshNode(transform, mesh, child, id);
	}

	void SceneRenderer::SubmitMesh(const glm::mat4& transform, Ref<Mesh> mesh, int id)
	{
		if (!mesh)
			return;

		RenderMeshNode(transform, mesh, mesh->GetRootNode(), id);
	}

	void SceneRenderer::SubmitQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, bool isTransparent, int id)
	{
		if (isTransparent)
			m_Renderer2D->DrawQuadTransparent(transform, texture, tilingfactor, tintcolor, id);
		else
			m_Renderer2D->DrawQuad(transform, texture, tilingfactor, tintcolor, id);
	}

	void SceneRenderer::SubmitFilledCircle(const glm::mat4& transform, float thickness, float fade, const glm::vec4& tintcolor, bool isTransparent, int id)
	{
		if (isTransparent)
			m_Renderer2D->DrawFilledCircleTransparent(transform, tintcolor, thickness, fade, id);
		else
			m_Renderer2D->DrawFilledCircle(transform, tintcolor, thickness, fade, id);
	}

	void SceneRenderer::Resize(uint32_t width, uint32_t height)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_ViewportWidth == width && m_ViewportHeight == height)
			return;

		//SK_CORE_INFO_TAG("SceneRenderer", "Resizing Scene Renderer {} ({}, {})", m_DebugName, width, height);

		m_ViewportWidth = width;
		m_ViewportHeight = height;
		m_NeedsResize = true;
	}

	void SceneRenderer::DrawSettings()
	{
		auto profiler = Application::Get().GetProfiler();
		profiler->Add("Geometry Pass", m_Renderer2D->GetStatistics().GeometryPassTime);
		profiler->Add("Opaque Geometry Pass", m_Renderer2D->GetStatistics().OpaqueGeometryTime);
		profiler->Add("OIT Geometry Pass", m_Renderer2D->GetStatistics().OITGeometryTime);

		const void* treeNodeID = this;
		if (ImGui::TreeNodeEx(treeNodeID, ImGuiTreeNodeFlags_CollapsingHeader, "Scene Renderer [%s]", m_DebugName.c_str()))
		{
			if (ImGui::TreeNodeEx("Settings", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				UI::BeginControlsGrid();

				if (UI::ControlColor("Clear Color", m_ClearColor))
					m_GeometryFrameBuffer->SetClearColor(m_ClearColor);

				UI::EndControls();

				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Statistics", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				UI::BeginControlsGrid();
				const auto& stats = m_Renderer2D->GetStatistics();
				UI::Property("DrawCalls", stats.DrawCalls);
				UI::Property("Quads", stats.QuadCount);
				UI::Property("Cirlces", stats.CircleCount);
				UI::Property("Lines", stats.LineCount);
				UI::Property("Glyphs", stats.GlyphCount);
				UI::Property("Vertices", stats.VertexCount);
				UI::Property("Indices", stats.IndexCount);
				UI::Property("Textures", stats.TextureCount);
				UI::EndControls();

				ImGui::TreePop();
			}

			UI::ScopedStyle treeNodeIndent(ImGuiStyleVar_IndentSpacing, 0.0f);

			if (ImGui::TreeNodeEx("Depth Buffer", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				Ref<Image2D> depthImage = m_Renderer2D->GetDepthImage();
				const float ratio = (float)depthImage->GetHeight() / (float)depthImage->GetWidth();
				const ImVec2 size = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x * ratio };
				ImGui::Image(depthImage->GetViewID(), size);
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Transparent Geometry", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				Ref<Image2D> image = m_Renderer2D->m_TransparentGeometryFrameBuffer->GetImage(0);
				const float ratio = (float)image->GetHeight() / (float)image->GetWidth();
				const ImVec2 size = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x * ratio };
				ImGui::Image(image->GetViewID(), size);
				ImGui::TreePop();
			}

			if (ImGui::TreeNodeEx("Revealage", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				Ref<Image2D> image = m_Renderer2D->m_TransparentGeometryFrameBuffer->GetImage(1);
				const float ratio = (float)image->GetHeight() / (float)image->GetWidth();
				const ImVec2 size = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x * ratio };
				ImGui::Image(image->GetViewID(), size);
				ImGui::TreePop();
			}
			
			if (ImGui::TreeNodeEx("Transparent Depth", ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanAvailWidth))
			{
				Ref<Image2D> image = m_Renderer2D->m_TransparentDepthBuffer->GetDepthImage();
				const float ratio = (float)image->GetHeight() / (float)image->GetWidth();
				const ImVec2 size = { ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().x * ratio };
				ImGui::Image(image->GetViewID(), size);
				ImGui::TreePop();
			}

		}
	}

}
