#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Components.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Math/Math.h"
#include "Shark/UI/UI.h"

#include "Shark/Input/Input.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	SceneRenderer::SceneRenderer(uint32_t width, uint32_t height, const std::string& debugName)
	{
		SceneRendererSpecification specification;
		specification.Width = width;
		specification.Height = height;
		specification.DebugName = debugName;
		Initialize(specification);
	}

	SceneRenderer::SceneRenderer(Ref<Scene> scene, const SceneRendererSpecification& specification)
		: m_Scene(scene)
	{
		Initialize(specification);
	}

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: m_Scene(scene)
	{
		SceneRendererSpecification specification;
		specification.Width = scene->GetViewportWidth();
		specification.Height = scene->GetViewportHeight();
		specification.DebugName = scene->GetName();
		Initialize(specification);
	}

	SceneRenderer::~SceneRenderer()
	{
		SK_PROFILE_FUNCTION();
	}

	void SceneRenderer::BeginScene(const SceneRendererCamera& camera)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_NeedsResize && m_Specification.Width != 0 && m_Specification.Height != 0)
		{
			SK_PROFILE_SCOPED("SceneRenderer::BeginScene Resize");

			m_GeometryFrameBuffer->Resize(m_Specification.Width, m_Specification.Height);
			m_ExternalCompositeFrameBuffer->Resize(m_Specification.Width, m_Specification.Height);
			m_Renderer2D->Resize(m_Specification.Width, m_Specification.Height);
			m_NeedsResize = false;
		}

		m_Statistics = {};

		m_MeshTransformCBIndex = 0;

		m_ViewProjection = camera.Projection * camera.View;
		m_View = camera.View;
		m_Projection = camera.Projection;
		m_CameraPosition = camera.Position;

		CBCamera cbCamera;
		cbCamera.ViewProj = m_ViewProjection;
		cbCamera.Position = m_CameraPosition;
		m_CBCamera->UploadData(Buffer::FromValue(cbCamera));

		CBSkybox skybox;
		skybox.SkyboxProjection = m_Projection * glm::mat4(glm::mat3(m_View));
		m_CBSkybox->UploadData(Buffer::FromValue(skybox));

		m_CommandBuffer->Begin();
		m_CommandBuffer->BeginTimeQuery(m_Timer);
		m_GeometryFrameBuffer->Clear(m_CommandBuffer);

		m_Renderer2D->BeginScene(m_ViewProjection);

		Renderer::BeginRenderPass(m_CommandBuffer, m_PBRPass);
	}

	void SceneRenderer::EndScene()
	{
		SK_PROFILE_FUNCTION();

		Renderer::EndRenderPass(m_CommandBuffer, m_PBRPass);

		SkyboxPass();

		m_CommandBuffer->EndTimeQuery(m_Timer);
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

	void SceneRenderer::SubmitPointLight(const glm::vec3& position, const glm::vec3& color, float intensity, float radius, float falloff)
	{
		CBLight cbLight;
		cbLight.Color = color;
		cbLight.Position = position;
		cbLight.Intensity = intensity;
		cbLight.Radius = radius;
		cbLight.Falloff = falloff;
		m_CBLight->UploadData(Buffer::FromValue(cbLight));
	}

	void SceneRenderer::SubmitMesh(const glm::mat4& transform, Ref<Mesh> mesh, uint32_t submeshIndex, int id)
	{
		SK_CORE_VERIFY(mesh);
		
		CBMeshData meshData;
		meshData.Transform = transform;
		meshData.ID = id;

		m_PBRPass->GetPipeline()->SetPushConstant(meshData);
		Renderer::RenderSubmesh(m_CommandBuffer, m_PBRPass->GetPipeline(), mesh, submeshIndex);

		m_Statistics.DrawCalls++;
		m_Statistics.VertexCount += mesh->GetMeshSource()->GetSubmeshes()[submeshIndex].VertexCount;
		m_Statistics.IndexCount += mesh->GetMeshSource()->GetSubmeshes()[submeshIndex].IndexCount;
	}

	void SceneRenderer::SubmitMesh(const glm::mat4& transform, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, int id)
	{
		SK_CORE_VERIFY(mesh);
		SK_CORE_VERIFY(material);

		CBMeshData meshData;
		meshData.Transform = transform;
		meshData.ID = id;

		m_PBRPass->GetPipeline()->SetPushConstant(meshData);
		Renderer::RenderSubmeshWithMaterial(m_CommandBuffer, m_PBRPass->GetPipeline(), mesh, submeshIndex, material);

		m_Statistics.DrawCalls++;
		m_Statistics.VertexCount += mesh->GetMeshSource()->GetSubmeshes()[submeshIndex].VertexCount;
		m_Statistics.IndexCount += mesh->GetMeshSource()->GetSubmeshes()[submeshIndex].IndexCount;
	}

	void SceneRenderer::SubmitQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingfactor, const glm::vec4& tintcolor, int id)
	{
		m_Renderer2D->DrawQuad(transform, texture, tilingfactor, tintcolor, id);
	}

	void SceneRenderer::SubmitFilledCircle(const glm::mat4& transform, float thickness, float fade, const glm::vec4& tintcolor, int id)
	{
		m_Renderer2D->DrawFilledCircle(transform, tintcolor, thickness, fade, id);
	}

	void SceneRenderer::Resize(uint32_t width, uint32_t height)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_Specification.Width == width && m_Specification.Height == height)
			return;

		//SK_CORE_INFO_TAG("SceneRenderer", "Resizing Scene Renderer {} ({}, {})", m_DebugName, width, height);

		m_Specification.Width = width;
		m_Specification.Height = height;
		m_NeedsResize = true;
	}

	void SceneRenderer::DrawSettings()
	{
		auto profiler = Application::Get().GetProfiler();
		if (profiler)
		{
			profiler->Add("[GPU] SceneRenderer", m_Timer->GetTime());
			profiler->Add("[GPU] Geometry Pass", m_Renderer2D->GetStatistics().GeometryPassTime);
		}

		const void* treeNodeID = this;
		if (ImGui::TreeNodeEx(treeNodeID, ImGuiTreeNodeFlags_CollapsingHeader, "Scene Renderer [%s]", m_Specification.DebugName.c_str()))
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

				UI::Property("Viewport", glm::vec2{ m_Specification.Width, m_Specification.Height });

				UI::Property("DrawCalls", m_Statistics.DrawCalls);
				UI::Property("Vertices", m_Statistics.VertexCount);
				UI::Property("Indices", m_Statistics.IndexCount);

				UI::Property("Mesh CBs", m_MeshTransformCBs.size());
				UI::Property("  In Use", m_MeshTransformCBIndex);
				UI::EndControls();

				if (ImGui::TreeNodeEx("Renderer2D", UI::DefaultThinHeaderFlags))
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

		}
	}

	void SceneRenderer::SkyboxPass()
	{
		Renderer::BeginRenderPass(m_CommandBuffer, m_SkyboxPass);
		Renderer::RenderCube(m_CommandBuffer, m_SkyboxPass->GetPipeline(), nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_SkyboxPass);
	}

	void SceneRenderer::Initialize(const SceneRendererSpecification& specification)
	{
		SK_PROFILE_FUNCTION();

		m_Specification = specification;

		m_CommandBuffer = RenderCommandBuffer::Create();
		m_Timer = GPUTimer::Create("SceneRenderer");

		m_CBCamera = ConstantBuffer::Create(sizeof(CBCamera));
		m_CBLight = ConstantBuffer::Create(sizeof(CBLight));
		m_CBSkybox = ConstantBuffer::Create(sizeof(CBSkybox));

		auto [environmentMap, irradianceMap] = Renderer::CreateEnvironmentMap("Resources/temp/pink_sunrise_4k.hdr");
		m_EnvironmentMap = environmentMap;
		m_IrradianceMap = irradianceMap;

		// Geometry
		{
			FrameBufferSpecification fbspecs;
			fbspecs.DebugName = "SceneRenderer Geometry";
			fbspecs.Width = specification.Width;
			fbspecs.Height = specification.Height;
			fbspecs.Atachments = { ImageFormat::RGBA8, ImageFormat::R32_SINT, ImageFormat::Depth };
			fbspecs.Atachments[1].BlendEnabled = false;
			fbspecs.ClearColor = m_ClearColor;
			fbspecs.IndipendendClearColor[1] = { -1.0f, -1.0f, -1.0f, -1.0f };

			if (specification.IsSwapchainTarget)
			{
				Ref<FrameBuffer> swapchainFramebuffer = Application::Get().GetWindow().GetSwapChain()->GetFrameBuffer();
				fbspecs.ExistingImages[0] = swapchainFramebuffer->GetImage(0);
			}

			m_GeometryFrameBuffer = FrameBuffer::Create(fbspecs);
			m_Renderer2D = Ref<Renderer2D>::Create(m_GeometryFrameBuffer);
		}


		// External Composite
		{
			FrameBufferSpecification fbspecs;
			fbspecs.DebugName = "SceneRenderer External Composite";
			fbspecs.Width = specification.Width;
			fbspecs.Height = specification.Height;
			fbspecs.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			fbspecs.Atachments = { ImageFormat::RGBA8, ImageFormat::R32_SINT, ImageFormat::Depth };
			fbspecs.Atachments[1].BlendEnabled = false;
			fbspecs.ExistingImages[0] = m_GeometryFrameBuffer->GetImage(0);
			fbspecs.ExistingImages[1] = m_GeometryFrameBuffer->GetImage(1);
			fbspecs.ExistingImages[2] = m_GeometryFrameBuffer->GetDepthImage();
			m_ExternalCompositeFrameBuffer = FrameBuffer::Create(fbspecs);
		}

		if (specification.IsSwapchainTarget)
		{
			Ref<SwapChain> swapchain = Application::Get().GetWindow().GetSwapChain();
			swapchain->AcknowledgeDependency(m_GeometryFrameBuffer);
			swapchain->AcknowledgeDependency(m_ExternalCompositeFrameBuffer);
		}

		// Mesh Pipeline
		{
			PipelineSpecification specification;
			specification.TargetFrameBuffer = m_GeometryFrameBuffer;
			specification.Shader = Renderer::GetShaderLibrary()->Get("SharkPBR");
			specification.Layout = {
				{ VertexDataType::Float3, "Position" },
				{ VertexDataType::Float3, "Normal" },
				{ VertexDataType::Float3, "Tangent" },
				{ VertexDataType::Float3, "Bitangent" },
				{ VertexDataType::Float2, "Texcoord" }
			};
			specification.DebugName = "PBR";

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(specification);
			renderPassSpecification.DebugName = "PBR";
			m_PBRPass = RenderPass::Create(renderPassSpecification);
			m_PBRPass->Set("u_Camera", m_CBCamera);
			m_PBRPass->Set("u_Light", m_CBLight);
			m_PBRPass->Set("u_IrradianceMap", m_IrradianceMap);
			SK_CORE_VERIFY(m_PBRPass->Validate());
			m_PBRPass->Bake();
		}

		// Skybox
		{
			PipelineSpecification specification;
			specification.TargetFrameBuffer = m_GeometryFrameBuffer;
			specification.Shader = Renderer::GetShaderLibrary()->Get("Skybox");
			specification.Layout = { VertexDataType::Float3, "Position" };
			specification.DebugName = "Skybox";

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(specification);
			renderPassSpecification.DebugName = specification.DebugName;
			m_SkyboxPass = RenderPass::Create(renderPassSpecification);
			m_SkyboxPass->Set("u_EnvironmentMap", m_EnvironmentMap);
			m_SkyboxPass->Set("u_Uniforms", m_CBSkybox);
			SK_CORE_VERIFY(m_SkyboxPass->Validate());
			m_SkyboxPass->Bake();
		}

	}

}
