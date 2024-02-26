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

		m_ViewProjection = camera.Projection * camera.View;
		m_View = camera.View;
		m_Projection = camera.Projection;
		m_CameraPosition = camera.Position;

		m_Lights.clear();
		m_Meshes.clear();

		m_Renderer2D->BeginScene(m_ViewProjection);
	}

	void SceneRenderer::EndScene()
	{
		SK_PROFILE_FUNCTION();

		m_CommandBuffer->Begin();
		m_CommandBuffer->BeginQuery(m_Timer);

		CBScene sceneData;
		sceneData.EnvironmentMapIntensity = m_EnvironmentMapIntensity;
		sceneData.LightCount = (uint32_t)m_Lights.size();
		m_CBScene->UploadData(Buffer::FromValue(sceneData));

		CBCamera cbCamera;
		cbCamera.ViewProj = m_ViewProjection;
		cbCamera.Position = m_CameraPosition;
		m_CBCamera->UploadData(Buffer::FromValue(cbCamera));

		CBSkybox skybox;
		skybox.SkyboxProjection = m_Projection * glm::mat4(glm::mat3(m_View));
		m_CBSkybox->UploadData(Buffer::FromValue(skybox));

		if (m_UpdateSkyboxSettings)
		{
			CBSkyboxSettings settings;
			settings.Lod = m_SkyboxLOD;
			m_CBSkyboxSettings->UploadData(Buffer::FromValue(settings));
		}

		if (m_Lights.size() > m_SBLights->GetCount())
		{
			uint32_t newCount = std::max({ m_SBLights->GetCount() * 2, (uint32_t)m_Lights.size(), 16u });
			m_SBLights->GetCount() = newCount;
			m_SBLights->Invalidate();
		}
		m_SBLights->Upload(Buffer::FromArray(m_Lights));

		ClearPass();
		GeometryPass();
		SkyboxPass();

		m_CommandBuffer->EndQuery(m_Timer);
		m_CommandBuffer->End();
		m_CommandBuffer->Execute(m_PipelineQuery);

		m_Renderer2D->EndScene();

		m_Statistics.GPUTime = Application::Get().GetGPUTime();
		m_Statistics.GeometryPass = m_GeometryPassTimer->GetTime();
		m_Statistics.SkyboxPass = m_SkyboxPassTimer->GetTime();
		m_PipelineStatistics = m_PipelineQuery->GetStatistics();
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
		Light& cbLight = m_Lights.emplace_back();
		cbLight.Color = color;
		cbLight.Position = position;
		cbLight.Intensity = intensity;
		cbLight.Radius = radius;
		cbLight.Falloff = falloff;
	}

	void SceneRenderer::SubmitMesh(const glm::mat4& transform, Ref<Mesh> mesh, uint32_t submeshIndex, int id)
	{
		SK_CORE_VERIFY(mesh);

		auto& meshData = m_Meshes.emplace_back();
		meshData.Mesh = mesh;
		meshData.Material = nullptr;
		meshData.SubmeshIndex = submeshIndex;
		meshData.Transform = transform;
		meshData.ID = id;
	}

	void SceneRenderer::SubmitMesh(const glm::mat4& transform, Ref<Mesh> mesh, uint32_t submeshIndex, Ref<Material> material, int id)
	{
		SK_CORE_VERIFY(mesh);
		SK_CORE_VERIFY(material);

		auto& meshData = m_Meshes.emplace_back();
		meshData.Mesh = mesh;
		meshData.Material = material;
		meshData.SubmeshIndex = submeshIndex;
		meshData.Transform = transform;
		meshData.ID = id;
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

	void SceneRenderer::ClearPass()
	{
		m_GeometryFrameBuffer->Clear(m_CommandBuffer);
	}

	void SceneRenderer::GeometryPass()
	{
		m_CommandBuffer->BeginQuery(m_GeometryPassTimer);
		Renderer::BeginRenderPass(m_CommandBuffer, m_PBRPass);

		for (const auto& mesh : m_Meshes)
		{
			MeshPushConstant PCMesh;
			PCMesh.Transform = mesh.Transform;
			PCMesh.ID = mesh.ID;
			m_PBRPass->GetPipeline()->SetPushConstant(PCMesh);

			if (mesh.Material)
				Renderer::RenderSubmeshWithMaterial(m_CommandBuffer, m_PBRPass->GetPipeline(), mesh.Mesh, mesh.SubmeshIndex, mesh.Material);
			else
				Renderer::RenderSubmesh(m_CommandBuffer, m_PBRPass->GetPipeline(), mesh.Mesh, mesh.SubmeshIndex);

			m_Statistics.DrawCalls++;
			m_Statistics.VertexCount += mesh.Mesh->GetMeshSource()->GetSubmeshes()[mesh.SubmeshIndex].VertexCount;
			m_Statistics.IndexCount += mesh.Mesh->GetMeshSource()->GetSubmeshes()[mesh.SubmeshIndex].IndexCount;
		}

		Renderer::EndRenderPass(m_CommandBuffer, m_PBRPass);
		m_CommandBuffer->EndQuery(m_GeometryPassTimer);
	}

	void SceneRenderer::SkyboxPass()
	{
		m_CommandBuffer->BeginQuery(m_SkyboxPassTimer);
		Renderer::BeginRenderPass(m_CommandBuffer, m_SkyboxPass);
		Renderer::RenderCube(m_CommandBuffer, m_SkyboxPass->GetPipeline(), nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_SkyboxPass);
		m_CommandBuffer->EndQuery(m_SkyboxPassTimer);
	}

	void SceneRenderer::Initialize(const SceneRendererSpecification& specification)
	{
		SK_PROFILE_FUNCTION();

		m_Specification = specification;

		m_CommandBuffer = RenderCommandBuffer::Create();

		m_PipelineQuery = GPUPipelineQuery::Create("Scene Renderer");
		m_Timer = GPUTimer::Create("SceneRenderer");
		m_GeometryPassTimer = GPUTimer::Create("Geometry Pass");
		m_SkyboxPassTimer = GPUTimer::Create("Skybox Pass");

		m_CBScene = ConstantBuffer::Create(sizeof(CBScene));
		m_CBCamera = ConstantBuffer::Create(sizeof(CBCamera));
		m_CBSkybox = ConstantBuffer::Create(sizeof(CBSkybox));
		m_CBSkyboxSettings = ConstantBuffer::Create(sizeof(CBSkyboxSettings));

		m_SBLights = StorageBuffer::Create(sizeof(Light), 16);


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
			m_PBRPass->Set("u_Scene", m_CBScene);
			m_PBRPass->Set("u_Lights", m_SBLights);
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
			m_SkyboxPass->Set("u_Settings", m_CBSkyboxSettings);
			SK_CORE_VERIFY(m_SkyboxPass->Validate());
			m_SkyboxPass->Bake();
		}

	}

}
