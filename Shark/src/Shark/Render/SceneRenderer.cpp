#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Scene/Scene.h"
#include "Shark/Scene/Components.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Math/Math.h"
#include "Shark/UI/UICore.h"

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
		m_GeometryPass->GetTargetFramebuffer()->SetClearColor(clearColor);
	}

	void SceneRenderer::BeginScene(const SceneRendererCamera& camera)
	{
		SK_PROFILE_FUNCTION();
		
		if (m_NeedsResize && m_Specification.Width != 0 && m_Specification.Height != 0)
		{
			m_GeometryPass->GetTargetFramebuffer()->Resize(m_Specification.Width, m_Specification.Height);
			m_SkyboxPass->GetTargetFramebuffer()->Resize(m_Specification.Width, m_Specification.Height);
			m_CompositePass->GetTargetFramebuffer()->Resize(m_Specification.Width, m_Specification.Height);

			m_SelectedGeometryPass->GetTargetFramebuffer()->Resize(m_Specification.Width, m_Specification.Height);
			m_TempFramebuffers[0]->Resize(m_Specification.Width, m_Specification.Height);
			m_TempFramebuffers[1]->Resize(m_Specification.Width, m_Specification.Height);
			m_JumpFloodInitPass->GetTargetFramebuffer()->Resize(m_Specification.Width, m_Specification.Height);
			m_JumpFloodPass[0]->GetTargetFramebuffer()->Resize(m_Specification.Width, m_Specification.Height);
			m_JumpFloodPass[1]->GetTargetFramebuffer()->Resize(m_Specification.Width, m_Specification.Height);
			m_JumpFloodCompositePass->GetTargetFramebuffer()->Resize(m_Specification.Width, m_Specification.Height);

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

		SkyboxPass();
		GeometryPass();

		if (m_Options.JumpFlood)
			JumpFloodPass();


		m_TimestampQueries.CompositePassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginEventMarker(m_CommandBuffer, "Composite");
		
		Renderer::BeginRenderPass(m_CommandBuffer, m_CompositePass);
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_CompositePass->GetPipeline(), nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_CompositePass);

		Renderer::EndEventMarker(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_TimestampQueries.CompositePassQuery);

		if (m_Options.JumpFlood)
		{
			Renderer::BeginEventMarker(m_CommandBuffer, "JumpFlood-Composite");

			Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodCompositePass);
			Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodCompositePass->GetPipeline(), m_JumpFloodCompositeMaterial);
			Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodCompositePass);

			Renderer::EndEventMarker(m_CommandBuffer);
		}

		m_CommandBuffer->EndTimestampQuery(m_TimestampQueries.TotalTimeQuery);
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();


		m_Statistics.GPUTime = m_CommandBuffer->GetTime(m_TimestampQueries.TotalTimeQuery);
		m_Statistics.GeometryPass = m_CommandBuffer->GetTime(m_TimestampQueries.GeometryPassQuery);
		m_Statistics.SkyboxPass = m_CommandBuffer->GetTime(m_TimestampQueries.SkyboxPassQuery);
		m_Statistics.CompositePass = m_CommandBuffer->GetTime(m_TimestampQueries.CompositePassQuery);
		m_Statistics.JumpFloodPass = m_CommandBuffer->GetTime(m_TimestampQueries.JumpFloodPassQuery);
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

		{
			auto& drawCommand = m_SelectedDrawList.emplace_back();
			drawCommand.Mesh = mesh;
			drawCommand.MeshSource = meshSource;
			drawCommand.SubmeshIndex = submeshIndex;
			drawCommand.Material = material;
			drawCommand.Transform = transform;
			drawCommand.ID = -1;
		}
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
		Renderer::Submit([buffer = m_CBScene, sceneData]()
		{
			buffer->RT_Upload(Buffer::FromValue(sceneData));
		});

		CBCamera cameraData;
		cameraData.ViewProj = m_ViewProjection;
		cameraData.Position = m_CameraPosition;
		Renderer::Submit([buffer = m_CBCamera, cameraData]()
		{
			buffer->RT_Upload(Buffer::FromValue(cameraData));
		});

		CBSkybox skybox;
		skybox.SkyboxProjection = m_Projection * glm::mat4(glm::mat3(m_View));

		CBSkyboxSettings settings;
		settings.Lod = m_Scene->GetSkyboxLod();
		settings.Intensity = m_Scene->GetEnvironmentIntesity();

		Renderer::Submit([instance = Ref(this), skybox, settings]()
		{
			instance->m_CBSkybox->RT_Upload(Buffer::FromValue(skybox));
			instance->m_CBSkyboxSettings->RT_Upload(Buffer::FromValue(settings));
		});

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
		compositeSettings.GammaCorrect = m_Options.GammaCorrect;
		compositeSettings.Exposure = m_Options.Exposure;
		Renderer::Submit([buffer = m_CBCompositeSettings, compositeSettings]()
		{
			buffer->RT_Upload(Buffer::FromValue(compositeSettings));
		});

		CBOutlineSettings outlineSettings;
		outlineSettings.Color = m_OutlineColor;
		outlineSettings.Color.a = m_OutlinePixelWidth;
		Renderer::Submit([buffer = m_CBOutlineSettings, outlineSettings]()
		{
			buffer->RT_Upload(Buffer::FromValue(outlineSettings));
		});
	}

	void SceneRenderer::GeometryPass()
	{
		m_TimestampQueries.GeometryPassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginEventMarker(m_CommandBuffer, "Geometry Pass");


		Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryPass);
		for (const auto& mesh : m_DrawList)
		{
			MeshPushConstant pcMesh;
			pcMesh.Transform = mesh.Transform;
			pcMesh.ID = mesh.ID;
			Renderer::Submit([commandBuffer = m_CommandBuffer, pipeline = m_GeometryPass->GetPipeline(), pcMesh]() { pipeline->RT_SetPushConstant(commandBuffer, Buffer::FromValue(pcMesh)); });
			Renderer::RenderSubmeshWithMaterial(m_CommandBuffer, m_GeometryPass->GetPipeline(), mesh.Mesh, mesh.MeshSource, mesh.SubmeshIndex, mesh.Material->GetMaterial());

			m_Statistics.DrawCalls++;
			m_Statistics.VertexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].VertexCount;
			m_Statistics.IndexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].IndexCount;
		}
		Renderer::EndRenderPass(m_CommandBuffer, m_GeometryPass);


		Renderer::BeginRenderPass(m_CommandBuffer, m_SelectedGeometryPass);
		for (const auto& mesh : m_SelectedDrawList)
		{
			MeshPushConstant pcMesh;
			pcMesh.Transform = mesh.Transform;
			pcMesh.ID = mesh.ID;
			Renderer::Submit([commandBuffer = m_CommandBuffer, pipeline = m_SelectedGeometryPass->GetPipeline(), pcMesh]() { pipeline->RT_SetPushConstant(commandBuffer, Buffer::FromValue(pcMesh)); });
			Renderer::RenderSubmeshWithMaterial(m_CommandBuffer, m_SelectedGeometryPass->GetPipeline(), mesh.Mesh, mesh.MeshSource, mesh.SubmeshIndex, m_SelectedGeometryMaterial);

			m_Statistics.DrawCalls++;
			m_Statistics.VertexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].VertexCount;
			m_Statistics.IndexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].IndexCount;
		}
		Renderer::EndRenderPass(m_CommandBuffer, m_SelectedGeometryPass);


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
		m_TimestampQueries.JumpFloodPassQuery = m_CommandBuffer->BeginTimestampQuery();
		Renderer::BeginEventMarker(m_CommandBuffer, "JumpFlood");

		Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodInitPass);
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodInitPass->GetPipeline(), m_JumpFloodInitMaterial);
		Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodInitPass);

		int steps = m_JumpFloodSteps;
		int step = glm::pow<int>(steps - 1, 2);
		int index = 0;
		Buffer vertexOverrides;
		Ref<FrameBuffer> passFB = m_JumpFloodPass[0]->GetTargetFramebuffer();
		glm::vec2 texelSize = { 1.0f / (float)passFB->GetSpecification().Width, 1.0f / (float)passFB->GetSpecification().Height };
		vertexOverrides.Allocate(sizeof(glm::vec2) + sizeof(int));
		vertexOverrides.Write(glm::value_ptr(texelSize), sizeof(glm::vec2));
		while (step != 0)
		{
			vertexOverrides.Write(&step, sizeof(int), sizeof(glm::vec2));

			Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodPass[index]);
			m_JumpFloodPass[index]->GetPipeline()->SetPushConstant(m_CommandBuffer, vertexOverrides);
			Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodPass[index]->GetSpecification().Pipeline, m_JumpFloodPassMaterial[index]);
			Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodPass[index]);

			index = (index + 1) % 2;
			step /= 2;
		}

		vertexOverrides.Release();

		Renderer::EndEventMarker(m_CommandBuffer);
		m_CommandBuffer->EndTimestampQuery(m_TimestampQueries.JumpFloodPassQuery);
	}

	void SceneRenderer::Initialize(const SceneRendererSpecification& specification)
	{
		SK_PROFILE_FUNCTION();

		m_Specification = specification;
		m_CommandBuffer = RenderCommandBuffer::Create();

		m_CBScene             = ConstantBuffer::Create(BufferUsage::Dynamic, sizeof(CBScene), "Scene");
		m_CBCamera            = ConstantBuffer::Create(BufferUsage::Dynamic, sizeof(CBCamera), "Camera");
		m_CBSkybox            = ConstantBuffer::Create(BufferUsage::Dynamic, sizeof(CBSkybox), "Skybox");
		m_CBSkyboxSettings    = ConstantBuffer::Create(BufferUsage::Dynamic, sizeof(CBSkyboxSettings), "SkyboxSettings");
		m_CBCompositeSettings = ConstantBuffer::Create(BufferUsage::Dynamic, sizeof(CBCompositeSettings), "CompositeSettings");
		m_CBOutlineSettings   = ConstantBuffer::Create(BufferUsage::Dynamic, sizeof(CBOutlineSettings), "OutlineSettings");

		m_SBPointLights       = StorageBuffer::Create(sizeof(PointLight), 16);
		m_SBDirectionalLights = StorageBuffer::Create(sizeof(DirectionalLight), LightEnvironment::MaxDirectionLights);


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

		FrameBufferSpecification mainFBSpecification;
		mainFBSpecification.DebugName = "Geometry";
		mainFBSpecification.Width = specification.Width;
		mainFBSpecification.Height = specification.Height;
		mainFBSpecification.Atachments = { ImageFormat::RGBA32Float, ImageFormat::R32SINT, ImageFormat::Depth32 };
		mainFBSpecification.ClearColor = m_ClearColor;
		mainFBSpecification.IndipendendClearColor[1] = { -1.0f, -1.0f, -1.0f, -1.0f };

		if (specification.IsSwapchainTarget)
		{
			Ref<FrameBuffer> swapchainFramebuffer = Application::Get().GetWindow().GetSwapChain()->GetFrameBuffer();
			mainFBSpecification.ExistingImages[0] = swapchainFramebuffer->GetImage(0);
		}

		Ref<FrameBuffer> clearFramebuffer = FrameBuffer::Create(mainFBSpecification);
		mainFBSpecification.ClearColorOnLoad = false;
		mainFBSpecification.ClearDepthOnLoad = false;
		mainFBSpecification.ExistingImages[0] = clearFramebuffer->GetImage(0);
		mainFBSpecification.ExistingImages[1] = clearFramebuffer->GetImage(1);
		mainFBSpecification.ExistingImages[2] = clearFramebuffer->GetDepthImage();
		Ref<FrameBuffer> loadFramebuffer = FrameBuffer::Create(mainFBSpecification);

		// Mesh
		{
			PipelineSpecification specification;
			specification.TargetFrameBuffer = loadFramebuffer;
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
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.TargetFrameBuffer = clearFramebuffer;
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

		// Selected Geometry
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.Width = specification.Width;
			framebufferSpecification.Height = specification.Height;
			framebufferSpecification.Atachments = { ImageFormat::RGBA32Float, ImageFormat::Depth32 };
			framebufferSpecification.DebugName = "Selected Geometry";
			framebufferSpecification.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			framebufferSpecification.ClearDepthValue = 1.0f;
			framebufferSpecification.ClearStencilValue = 0;

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.TargetFrameBuffer = FrameBuffer::Create(framebufferSpecification);
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SelectedGeometry");
			pipelineSpecification.Layout = layout;
			pipelineSpecification.DebugName = framebufferSpecification.DebugName;

			pipelineSpecification.EnableStencil = true;
			pipelineSpecification.StencilRef = 1;
			pipelineSpecification.StencilReadMask = 1;
			pipelineSpecification.StencilWriteMask = 1;
			pipelineSpecification.StencilComparisonOperator = CompareOperator::NotEqual;
			pipelineSpecification.StencilPassOperation = StencilOperation::Replace;

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
			renderPassSpecification.DebugName = pipelineSpecification.DebugName;

			m_SelectedGeometryPass = RenderPass::Create(renderPassSpecification);
			m_SelectedGeometryPass->Set("u_Camera", m_CBCamera);
			SK_CORE_VERIFY(m_SelectedGeometryPass->Validate());
			m_SelectedGeometryPass->Bake();

			m_SelectedGeometryMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
		}

		// Composite
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.Width = specification.Width;
			framebufferSpecification.Height = specification.Height;
			framebufferSpecification.Atachments = { ImageFormat::RGBA32Float, ImageFormat::R32SINT, ImageFormat::Depth32 };
			framebufferSpecification.ExistingImages[1] = m_GeometryPass->GetOutput(1);
			framebufferSpecification.ExistingImages[2] = m_GeometryPass->GetDepthOutput();
			framebufferSpecification.ClearColorOnLoad = false;
			framebufferSpecification.ClearDepthOnLoad = false;
			framebufferSpecification.DebugName = "Composite";

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.TargetFrameBuffer = FrameBuffer::Create(framebufferSpecification);
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Composite");
			pipelineSpecification.Layout = {
				{ VertexDataType::Float3, "Position" },
				{ VertexDataType::Float2, "TexCoord" },
			};
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

		// Temporary framebuffers
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.Width = m_Specification.Width;
			framebufferSpecification.Height = m_Specification.Height;
			framebufferSpecification.Atachments = { ImageFormat::RGBA32Float };
			framebufferSpecification.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			framebufferSpecification.BlendMode = FrameBufferBlendMode::OneZero;
			framebufferSpecification.DebugName = "Temporary";

			for (uint32_t i = 0; i < 2; i++)
				m_TempFramebuffers.push_back(FrameBuffer::Create(framebufferSpecification));
		}

		// Jump Flood Init
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.DebugName = "JumpFlood-Init";
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFloodInit");
			pipelineSpecification.TargetFrameBuffer = m_TempFramebuffers[0];
			pipelineSpecification.Layout = {
				{ VertexDataType::Float3, "Position" },
				{ VertexDataType::Float2, "TexCoord" }
			};

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
			renderPassSpecification.DebugName = "JumpFlood-Init";
			m_JumpFloodInitPass = RenderPass::Create(renderPassSpecification);
			m_JumpFloodInitPass->Set("u_Texture", m_SelectedGeometryPass->GetOutput(0));
			SK_CORE_VERIFY(m_JumpFloodInitPass->Validate());
			m_JumpFloodInitPass->Bake();

			m_JumpFloodInitMaterial = Material::Create(pipelineSpecification.Shader);

			const char* passNames[2] = { "EvenPass", "OddPass" };
			for (uint32_t i = 0; i < 2; i++)
			{
				pipelineSpecification.DebugName = fmt::format("JumpFlood-{}", passNames[i]);
				pipelineSpecification.TargetFrameBuffer = m_TempFramebuffers[(i + 1) % 2];
				pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFloodPass");

				renderPassSpecification.DebugName = pipelineSpecification.DebugName;
				renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
				m_JumpFloodPass[i] = RenderPass::Create(renderPassSpecification);
				m_JumpFloodPass[i]->Set("u_Texture", m_TempFramebuffers[i]->GetImage());
				SK_CORE_VERIFY(m_JumpFloodPass[i]->Validate());
				m_JumpFloodPass[i]->Bake();

				m_JumpFloodPassMaterial[i] = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
			}

			FrameBufferSpecification fbSpec;
			fbSpec.Width = m_Specification.Width;
			fbSpec.Height = m_Specification.Height;
			fbSpec.Atachments = { ImageFormat::RGBA32Float };
			fbSpec.ExistingImages[0] = m_CompositePass->GetOutput(0);
			fbSpec.ClearColorOnLoad = false;
			fbSpec.DebugName = "JumpFlood-Composite";

			pipelineSpecification.TargetFrameBuffer = FrameBuffer::Create(fbSpec);
			pipelineSpecification.DebugName = fbSpec.DebugName;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("JumpFloodComposite");
			pipelineSpecification.DepthEnabled = false;
			
			renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
			renderPassSpecification.DebugName = pipelineSpecification.DebugName;
			m_JumpFloodCompositePass = RenderPass::Create(renderPassSpecification);
			m_JumpFloodCompositePass->Set("u_Texture", m_TempFramebuffers[1]->GetImage());
			m_JumpFloodCompositePass->Set("u_Settings", m_CBOutlineSettings);
			SK_CORE_VERIFY(m_JumpFloodCompositePass->Validate());
			m_JumpFloodCompositePass->Bake();

			m_JumpFloodCompositeMaterial = Material::Create(pipelineSpecification.Shader, pipelineSpecification.DebugName);
		}

		if (specification.IsSwapchainTarget)
		{
			Ref<SwapChain> swapchain = Application::Get().GetWindow().GetSwapChain();
			swapchain->AcknowledgeDependency(m_GeometryPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer);
			swapchain->AcknowledgeDependency(m_SkyboxPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer);
		}

		m_Renderer2D = Ref<Renderer2D>::Create(m_CompositePass->GetTargetFramebuffer());
	}

}
