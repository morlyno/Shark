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

#include "Platform/DirectX11/DirectXContext.h"
#include "Platform/DirectX11/DirectXImage.h"
#include "Platform/DirectX11/DirectXRenderCommandBuffer.h"
#include "Platform/DirectX11/DirectXShader.h"
#include "Platform/DirectX11/DirectXConstantBuffer.h"

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

	void SceneRenderer::SetClearColor(const glm::vec4& clearColor)
	{
		m_ClearColor = clearColor;
		m_GeometryPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer->SetClearColor(clearColor);
	}

	void SceneRenderer::BeginScene(const SceneRendererCamera& camera)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_NeedsResize && m_Specification.Width != 0 && m_Specification.Height != 0)
		{
			m_GeometryPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer->Resize(m_Specification.Width, m_Specification.Height);
			m_SkyboxPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer->Resize(m_Specification.Width, m_Specification.Height);
			m_CompositePass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer->Resize(m_Specification.Width, m_Specification.Height);

			m_NearestPointBuffer->Resize(m_Specification.Width, m_Specification.Height);
			m_NearestPointPingPongBuffer->Resize(m_Specification.Width, m_Specification.Height);
			m_JumpFloodStencilPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer->Resize(m_Specification.Width, m_Specification.Height);
			m_JumpFloodInitPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer->Resize(m_Specification.Width, m_Specification.Height);
			m_JumpFloodPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer->Resize(m_Specification.Width, m_Specification.Height);
			m_JumpFloodOutlinePass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer->Resize(m_Specification.Width, m_Specification.Height);

			m_Renderer2D->Resize(m_Specification.Width, m_Specification.Height);
			m_NeedsResize = false;
		}

		m_Statistics = Statistics();
		m_TimestampQueries = TimestampQueries();

		m_ViewProjection = camera.Projection * camera.View;
		m_View = camera.View;
		m_Projection = camera.Projection;
		m_CameraPosition = camera.Position;

		m_DrawList.clear();
		m_SelectedDrawList.clear();
	}

	void SceneRenderer::EndScene()
	{
		SK_PROFILE_FUNCTION();

		PreRender();

		m_CommandBuffer->Begin();
		m_TimestampQueries.TotalTimeQuery = m_CommandBuffer->BeginTimestampQuery();

		GeometryPass();
		if (m_Options.SkyboxPass)
			SkyboxPass();

		JumpFloodPass();

		m_TimestampQueries.CompositePassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginRenderPass(m_CommandBuffer, m_CompositePass);
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_CompositePass->GetPipeline(), nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_CompositePass);
		m_CommandBuffer->EndTimestampQuery(m_TimestampQueries.CompositePassQuery);

		m_CommandBuffer->EndTimestampQuery(m_TimestampQueries.TotalTimeQuery);
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();

		m_Statistics.GPUTime = m_CommandBuffer->GetTime(m_TimestampQueries.TotalTimeQuery);
		m_Statistics.GeometryPass = m_CommandBuffer->GetTime(m_TimestampQueries.GeometryPassQuery);
		m_Statistics.SkyboxPass = m_CommandBuffer->GetTime(m_TimestampQueries.SkyboxPassQuery);
		m_Statistics.CompositePass = m_CommandBuffer->GetTime(m_TimestampQueries.CompositePassQuery);
		m_PipelineStatistics = m_CommandBuffer->GetPipelineStatistics();
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialAsset> material, const glm::mat4& transform, int id)
	{
		SK_CORE_VERIFY(mesh);
		SK_CORE_VERIFY(material);

		// set concrete textures in the material
		material->Invalidate();

		auto& meshData = m_DrawList.emplace_back();
		meshData.Mesh = mesh;
		meshData.MeshSource = meshSource;
		meshData.SubmeshIndex = submeshIndex;
		meshData.Material = material;
		meshData.Transform = transform;
		meshData.ID = id;
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<MaterialAsset> material, const glm::mat4& transform)
	{
		SK_CORE_VERIFY(mesh);
		SK_CORE_VERIFY(material);

		// set concrete textures in the material
		material->Invalidate();

		auto& meshData = m_SelectedDrawList.emplace_back();
		meshData.Mesh = mesh;
		meshData.MeshSource = meshSource;
		meshData.SubmeshIndex = submeshIndex;
		meshData.Material = material;
		meshData.Transform = transform;
		meshData.ID = -1;
	}

	void SceneRenderer::PreRender()
	{
		Ref<Environment> environment = m_Scene->GetEnvironment();
		m_GeometryPass->Set("u_IrradianceMap", environment->GetIrradianceMap());
		m_GeometryPass->Set("u_RadianceMap", environment->GetRadianceMap());
		m_GeometryPass->Update();

		m_SkyboxPass->Set("u_EnvironmentMap", environment->GetRadianceMap());
		m_SkyboxPass->Update();

		CBScene sceneData;
		sceneData.EnvironmentMapIntensity = m_Scene->GetEnvironmentIntesity();
		sceneData.PointLightCount = (uint32_t)m_Scene->GetPointLights().size();
		sceneData.DirectionalLightCount = m_Scene->GetLightEnvironment().DirectionalLightCount;
		m_CBScene->UploadData(Buffer::FromValue(sceneData));

		CBCamera cbCamera;
		cbCamera.ViewProj = m_ViewProjection;
		cbCamera.Position = m_CameraPosition;
		m_CBCamera->UploadData(Buffer::FromValue(cbCamera));

		if (m_Options.SkyboxPass)
		{
			CBSkybox skybox;
			skybox.SkyboxProjection = m_Projection * glm::mat4(glm::mat3(m_View));
			m_CBSkybox->UploadData(Buffer::FromValue(skybox));

			CBSkyboxSettings settings;
			settings.Lod = m_Scene->GetSkyboxLod();
			settings.Intensity = m_Scene->GetEnvironmentIntesity();
			m_CBSkyboxSettings->UploadData(Buffer::FromValue(settings));
		}

		const auto& pointLights = m_Scene->GetPointLights();
		if (pointLights.size() > m_SBPointLights->GetCount())
		{
			uint32_t newCount = std::max({ m_SBPointLights->GetCount() * 2, (uint32_t)pointLights.size(), 16u });
			m_SBPointLights->GetCount() = newCount;
			m_SBPointLights->Invalidate();
		}

		m_SBPointLights->Upload(Buffer::FromArray(pointLights));
		m_SBDirectionalLights->Upload(Buffer::FromArray(m_Scene->GetLightEnvironment().DirectionalLights));

		CBCompositeSettings compositeSettings;
		compositeSettings.Tonemap = m_Options.Tonemap;
		compositeSettings.Exposure = m_Options.Exposure;
		m_CBCompositeSettings->UploadData(Buffer::FromValue(compositeSettings));

		JumpFloodUniforms jumpFlood;
		jumpFlood.TexelSize = { 1.0f / m_Specification.Width, 1.0f / m_Specification.Height, m_Specification.Width, m_Specification.Height };
		jumpFlood.OutlineColor = m_OutlineColor;
		jumpFlood.FrameBufferSize = { m_Specification.Width, m_Specification.Height };
		jumpFlood.OutlineWidth = m_OutlinePixelWidth;
		m_JumpFloodUniforms->UploadData(Buffer::FromValue(jumpFlood));
	}

	void SceneRenderer::GeometryPass()
	{
		m_TimestampQueries.GeometryPassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginEventMarker(m_CommandBuffer, "Geometry Pass");
		Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryPass, true);

		for (const auto& mesh : m_DrawList)
		{
			MeshPushConstant pcMesh;
			pcMesh.Transform = mesh.Transform;
			pcMesh.ID = mesh.ID;
			m_GeometryPass->GetPipeline()->SetPushConstant(pcMesh);

			Renderer::RenderSubmeshWithMaterial(m_CommandBuffer, m_GeometryPass->GetPipeline(), mesh.Mesh, mesh.MeshSource, mesh.SubmeshIndex, mesh.Material->GetMaterial());

			m_Statistics.DrawCalls++;
			m_Statistics.VertexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].VertexCount;
			m_Statistics.IndexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].IndexCount;
		}

		Renderer::EndRenderPass(m_CommandBuffer, m_GeometryPass);
		Renderer::EndEventMarker(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_TimestampQueries.GeometryPassQuery);
	}

	void SceneRenderer::SkyboxPass()
	{
		m_TimestampQueries.SkyboxPassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginEventMarker(m_CommandBuffer, "Skybox Pass");
		Renderer::BeginRenderPass(m_CommandBuffer, m_SkyboxPass);
		Renderer::RenderCube(m_CommandBuffer, m_SkyboxPass->GetPipeline(), nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_SkyboxPass);
		Renderer::EndEventMarker(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_TimestampQueries.SkyboxPassQuery);
	}

	void SceneRenderer::JumpFloodPass()
	{
		Renderer::BeginEventMarker(m_CommandBuffer, "Jump Flood");

		Renderer::BeginEventMarker(m_CommandBuffer, "Fill Buffers Pass");
		Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodStencilPass, true);
		for (const auto& mesh : m_SelectedDrawList)
		{
			m_JumpFloodStencilPass->GetPipeline()->SetPushConstant(Buffer::FromValue(mesh.Transform));
			Renderer::RenderSubmeshWithMaterial(m_CommandBuffer, m_JumpFloodStencilPass->GetPipeline(), mesh.Mesh, mesh.MeshSource, mesh.SubmeshIndex, nullptr);
		}
		Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodStencilPass);
		Renderer::EndEventMarker(m_CommandBuffer);

		m_NearestPointBuffer->Clear(m_CommandBuffer);
		m_NearestPointPingPongBuffer->Clear(m_CommandBuffer);

		Renderer::BeginEventMarker(m_CommandBuffer, "Init Pass");
		Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodInitPass);
		m_JumpFloodInitMaterial->Set("u_MainTexture", m_JumpFloodStencilPass->GetOutput(0));
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodInitPass->GetPipeline(), m_JumpFloodInitMaterial);
		Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodInitPass);
		Renderer::EndEventMarker(m_CommandBuffer);


		Renderer::BeginEventMarker(m_CommandBuffer, "Jump Flood Pass");
		Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodPass);
		int numMips = (int)glm::ceil(glm::log2(m_OutlinePixelWidth + 1.0f));
		int jfaIter = numMips - 1;

		m_NearestPointMaterial->Set("u_MainTexture", m_NearestPointBuffer->GetImage());
		m_NearestPointPingPongMaterial->Set("u_MainTexture", m_NearestPointPingPongBuffer->GetImage());

		for (int i = jfaIter; i >= 0; i--)
		{
			float stepWidth = glm::pow(2.0f, (float)i) + 0.5f;

			m_JumpFloodPass->GetPipeline()->SetFrameBuffer(m_NearestPointPingPongBuffer);
			m_JumpFloodPass->GetPipeline()->SetPushConstant(glm::ivec2(stepWidth, 0.0f));
			Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodPass->GetPipeline(), m_NearestPointMaterial);

			m_JumpFloodPass->GetPipeline()->SetFrameBuffer(m_NearestPointBuffer);
			m_JumpFloodPass->GetPipeline()->SetPushConstant(glm::ivec2(0.0f, stepWidth));
			Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodPass->GetPipeline(), m_NearestPointPingPongMaterial);
		}
		Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodPass);
		Renderer::EndEventMarker(m_CommandBuffer);

		Renderer::BeginEventMarker(m_CommandBuffer, "Outline Pass");
		Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodOutlinePass);
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodOutlinePass->GetPipeline(), nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodOutlinePass);
		Renderer::EndEventMarker(m_CommandBuffer);

		Renderer::EndEventMarker(m_CommandBuffer);
	}

	void SceneRenderer::Initialize(const SceneRendererSpecification& specification)
	{
		SK_PROFILE_FUNCTION();

		m_Specification = specification;

		m_CommandBuffer = RenderCommandBuffer::Create();

		m_CBScene = ConstantBuffer::Create(sizeof(CBScene));
		m_CBCamera = ConstantBuffer::Create(sizeof(CBCamera));
		m_CBSkybox = ConstantBuffer::Create(sizeof(CBSkybox));
		m_CBSkyboxSettings = ConstantBuffer::Create(sizeof(CBSkyboxSettings));
		m_CBCompositeSettings = ConstantBuffer::Create(sizeof(CBCompositeSettings));
		m_SBPointLights = StorageBuffer::Create(sizeof(PointLight), 16);
		m_SBDirectionalLights = StorageBuffer::Create(sizeof(DirectionalLight), LightEnvironment::MaxDirectionLights);
		m_JumpFloodUniforms = ConstantBuffer::Create(sizeof(JumpFloodUniforms));


		VertexLayout layout = {
			{ VertexDataType::Float3, "Position" },
			{ VertexDataType::Float3, "Normal" },
			{ VertexDataType::Float3, "Tangent" },
			{ VertexDataType::Float3, "Bitangent" },
			{ VertexDataType::Float2, "Texcoord" }
		};

		VertexLayout skyboxLayout = {
			{ VertexDataType::Float3, "Position" }
		};

		// Mesh
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.DebugName = "SceneRenderer Geometry";
			framebufferSpecification.Width = specification.Width;
			framebufferSpecification.Height = specification.Height;
			framebufferSpecification.Atachments = { ImageFormat::RGBA32Float, ImageFormat::R32SINT, ImageFormat::Depth32 };
			framebufferSpecification.ClearColor = m_ClearColor;
			framebufferSpecification.IndipendendClearColor[1] = { -1.0f, -1.0f, -1.0f, -1.0f };

			if (specification.IsSwapchainTarget)
			{
				Ref<FrameBuffer> swapchainFramebuffer = Application::Get().GetWindow().GetSwapChain()->GetFrameBuffer();
				framebufferSpecification.ExistingImages[0] = swapchainFramebuffer->GetImage(0);
			}

			PipelineSpecification specification;
			specification.TargetFrameBuffer = FrameBuffer::Create(framebufferSpecification);
			specification.Shader = Renderer::GetShaderLibrary()->Get("SharkPBR");
			specification.Layout = layout;
			specification.DebugName = "PBR";

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(specification);
			renderPassSpecification.DebugName = "PBR";
			m_GeometryPass = RenderPass::Create(renderPassSpecification);
			m_GeometryPass->Set("u_Camera", m_CBCamera);
			m_GeometryPass->Set("u_Scene", m_CBScene);
			m_GeometryPass->Set("u_PointLights", m_SBPointLights);
			m_GeometryPass->Set("u_DirectionalLights", m_SBDirectionalLights);
			m_GeometryPass->Set("u_IrradianceMap", Renderer::GetBlackTextureCube());
			m_GeometryPass->Set("u_RadianceMap", Renderer::GetBlackTextureCube());
			m_GeometryPass->Set("u_BRDFLUTTexture", Renderer::GetBRDFLUTTexture());
			SK_CORE_VERIFY(m_GeometryPass->Validate());
			m_GeometryPass->Bake();
		}

		// Skybox
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.Width = specification.Width;
			framebufferSpecification.Height = specification.Height;
			framebufferSpecification.Atachments = { ImageFormat::RGBA32Float, ImageFormat::Depth32 };
			framebufferSpecification.ExistingImages[0] = m_GeometryPass->GetOutput(0);
			framebufferSpecification.ExistingImages[1] = m_GeometryPass->GetDepthOutput();
			framebufferSpecification.DebugName = "Skybox";

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.TargetFrameBuffer = FrameBuffer::Create(framebufferSpecification);
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Skybox");
			pipelineSpecification.Layout = skyboxLayout;
			pipelineSpecification.DebugName = "Skybox";

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
			renderPassSpecification.DebugName = pipelineSpecification.DebugName;
			m_SkyboxPass = RenderPass::Create(renderPassSpecification);
			m_SkyboxPass->Set("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_SkyboxPass->Set("u_Uniforms", m_CBSkybox);
			m_SkyboxPass->Set("u_Settings", m_CBSkyboxSettings);
			SK_CORE_VERIFY(m_SkyboxPass->Validate());
			m_SkyboxPass->Bake();
		}

		// Jump Flood Fill Buffers
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.Width = specification.Width;
			framebufferSpecification.Height = specification.Height;
			framebufferSpecification.Atachments = { ImageFormat::R8UNorm, ImageFormat::Depth24UNormStencil8UINT };
			framebufferSpecification.DebugName = "Jump Flood Buffers";
			framebufferSpecification.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			framebufferSpecification.ClearStencil = 0;
			Ref<FrameBuffer> framebuffer = FrameBuffer::Create(framebufferSpecification);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.TargetFrameBuffer = framebuffer;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFloodFillBuffers");
			pipelineSpecification.Layout = layout;
			pipelineSpecification.DebugName = "Jump Flood Fill Buffers";

			pipelineSpecification.BackFaceCulling = false;
			pipelineSpecification.WriteDepth = false;

			pipelineSpecification.EnableStencil = true;
			pipelineSpecification.StencilRef = 1;
			pipelineSpecification.StencilReadMask = 1;
			pipelineSpecification.StencilWriteMask = 1;
			pipelineSpecification.StencilComparisonOperator = CompareOperator::NotEqual;
			pipelineSpecification.StencilPassOperation = StencilOperation::Replace;

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
			renderPassSpecification.DebugName = pipelineSpecification.DebugName;
			m_JumpFloodStencilPass = RenderPass::Create(renderPassSpecification);
			m_JumpFloodStencilPass->Set("u_Camera", m_CBCamera);
			SK_CORE_VERIFY(m_JumpFloodStencilPass->Validate());
			m_JumpFloodStencilPass->Bake();
		}

		// Jump Flood Init
		{
			FrameBufferSpecification nearesPointFramebufferSpecification;
			nearesPointFramebufferSpecification.Width = specification.Width;
			nearesPointFramebufferSpecification.Height = specification.Height;
			nearesPointFramebufferSpecification.Atachments = { ImageFormat::RG16SNorm };
			nearesPointFramebufferSpecification.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

			nearesPointFramebufferSpecification.DebugName = "Nearest Point";
			m_NearestPointBuffer = FrameBuffer::Create(nearesPointFramebufferSpecification);
			nearesPointFramebufferSpecification.DebugName = "Nearest Point Ping Pong";
			m_NearestPointPingPongBuffer = FrameBuffer::Create(nearesPointFramebufferSpecification);


			PipelineSpecification initPipelineSpecification;
			initPipelineSpecification.TargetFrameBuffer = m_NearestPointBuffer;
			initPipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFloodInit");
			initPipelineSpecification.Layout = { VertexDataType::Float2, "Position" };
			initPipelineSpecification.DebugName = "Jump Flood Init";
			initPipelineSpecification.BackFaceCulling = false;
			initPipelineSpecification.WriteDepth = false;

			RenderPassSpecification initRenderPassSpecification;
			initRenderPassSpecification.Pipeline = Pipeline::Create(initPipelineSpecification);
			initRenderPassSpecification.DebugName = "Jump Flood Init";
			m_JumpFloodInitPass = RenderPass::Create(initRenderPassSpecification);
			m_JumpFloodInitPass->Set("u_Camera", m_CBCamera);
			m_JumpFloodInitPass->Set("u_Uniforms", m_JumpFloodUniforms);
			SK_CORE_VERIFY(m_JumpFloodInitPass->Validate());
			m_JumpFloodInitPass->Bake();

			m_JumpFloodInitMaterial = Material::Create(initPipelineSpecification.Shader);
		}

		// Jump Flood Pass
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.TargetFrameBuffer = m_NearestPointPingPongBuffer;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFlood");
			pipelineSpecification.Layout = { VertexDataType::Float2, "Position" };
			pipelineSpecification.DebugName = "Jump Flood";

			pipelineSpecification.BackFaceCulling = false;
			pipelineSpecification.WriteDepth = false;

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
			renderPassSpecification.DebugName = "Jump Flood";
			m_JumpFloodPass = RenderPass::Create(renderPassSpecification);
			m_JumpFloodPass->Set("u_Camera", m_CBCamera);
			m_JumpFloodPass->Set("u_Uniforms", m_JumpFloodUniforms);
			SK_CORE_VERIFY(m_JumpFloodPass->Validate());
			m_JumpFloodPass->Bake();

			m_NearestPointMaterial = Material::Create(pipelineSpecification.Shader);
			m_NearestPointPingPongMaterial = Material::Create(pipelineSpecification.Shader);

		}

		// Jump Flood Composite
		{
			FrameBufferSpecification compositeFramebufferSpecification;
			compositeFramebufferSpecification.Width = specification.Width;
			compositeFramebufferSpecification.Height = specification.Height;
			compositeFramebufferSpecification.Atachments = { ImageFormat::RGBA32Float, ImageFormat::Depth24UNormStencil8UINT };
			compositeFramebufferSpecification.Atachments[0].BlendEnabled = true;
			compositeFramebufferSpecification.ExistingImages[0] = m_GeometryPass->GetOutput(0);
			compositeFramebufferSpecification.ExistingImages[1] = m_JumpFloodStencilPass->GetDepthOutput();
			compositeFramebufferSpecification.DebugName = "Jump Flood Composite";

			PipelineSpecification compositePipelineSpecification;
			compositePipelineSpecification.TargetFrameBuffer = FrameBuffer::Create(compositeFramebufferSpecification);
			compositePipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFloodOutline");
			compositePipelineSpecification.Layout = { VertexDataType::Float2, "Position" };
			compositePipelineSpecification.DebugName = "Jump Flood Composite";

			compositePipelineSpecification.BackFaceCulling = false;
			compositePipelineSpecification.WriteDepth = false;

			compositePipelineSpecification.EnableStencil = true;
			compositePipelineSpecification.StencilRef = 1;
			compositePipelineSpecification.StencilReadMask = 1;
			compositePipelineSpecification.StencilWriteMask = 1;
			compositePipelineSpecification.StencilComparisonOperator = CompareOperator::NotEqual;
			compositePipelineSpecification.StencilPassOperation = StencilOperation::Zero;
			compositePipelineSpecification.StencilFailOperation = StencilOperation::Zero;

			RenderPassSpecification compositeRenderPassSpecification;
			compositeRenderPassSpecification.Pipeline = Pipeline::Create(compositePipelineSpecification);
			compositeRenderPassSpecification.DebugName = compositePipelineSpecification.DebugName;
			m_JumpFloodOutlinePass = RenderPass::Create(compositeRenderPassSpecification);
			m_JumpFloodOutlinePass->Set("u_Camera", m_CBCamera);
			m_JumpFloodOutlinePass->Set("u_Uniforms", m_JumpFloodUniforms);
			SK_CORE_VERIFY(m_JumpFloodOutlinePass->Validate());
			m_JumpFloodOutlinePass->Bake();
		}

		// Composite
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.Width = specification.Width;
			framebufferSpecification.Height = specification.Height;
			framebufferSpecification.Atachments = { ImageFormat::RGBA8UNorm, ImageFormat::R32SINT, ImageFormat::Depth32 };
			framebufferSpecification.ExistingImages[1] = m_GeometryPass->GetOutput(1);
			framebufferSpecification.ExistingImages[2] = m_GeometryPass->GetDepthOutput();
			framebufferSpecification.DebugName = "Composite";

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.TargetFrameBuffer = FrameBuffer::Create(framebufferSpecification);
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Composite");
			pipelineSpecification.Layout = { VertexDataType::Float2, "Position" };
			pipelineSpecification.DebugName = framebufferSpecification.DebugName;
			pipelineSpecification.WriteDepth = false;

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
			renderPassSpecification.DebugName = pipelineSpecification.DebugName;
			m_CompositePass = RenderPass::Create(renderPassSpecification);
			m_CompositePass->Set("u_Settings", m_CBCompositeSettings);
			m_CompositePass->Set("u_Input", m_SkyboxPass->GetOutput(0));
			SK_CORE_VERIFY(m_CompositePass->Validate());
			m_CompositePass->Bake();
		}

		if (specification.IsSwapchainTarget)
		{
			Ref<SwapChain> swapchain = Application::Get().GetWindow().GetSwapChain();
			swapchain->AcknowledgeDependency(m_GeometryPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer);
			swapchain->AcknowledgeDependency(m_SkyboxPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer);
		}

		m_Renderer2D = Ref<Renderer2D>::Create(m_CompositePass);
	}

}
