#include "skpch.h"
#include "SceneRenderer.h"

#include "Shark/Scene/Scene.h"

#include "Shark/Render/Renderer.h"
#include "Shark/Render/Renderer2D.h"
#include "Shark/Math/Math.h"

#include "Shark/Debug/Profiler.h"
#include <glm/gtx/optimum_pow.hpp>
#include "Shark/Asset/AssetManager.h"

namespace Shark {

	SceneRenderer::SceneRenderer(uint32_t width, uint32_t height, const std::string& debugName)
	{
		SceneRendererSpecification specification;
		specification.Width = width;
		specification.Height = height;
		specification.DebugName = debugName;
		Initialize(specification);
	}

	SceneRenderer::SceneRenderer(const SceneRendererSpecification& specification)
	{
		Initialize(specification);
	}

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
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

	void SceneRenderer::BeginScene(Ref<Scene> scene, const SceneRendererCamera& camera)
	{
		SK_PROFILE_FUNCTION();
		
		m_Scene = scene;

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

			// #Renderer #Investigate submitting should be handled by the backend, not the user
			Renderer::Submit([this]()
			{
				// Update invalidated binding sets
				m_GeometryPass->Update();
				m_SkyboxPass->Update();
				m_CompositePass->Update();
				m_SelectedGeometryPass->Update();
				m_JumpFloodInitPass->Update();
				m_JumpFloodPass[0]->Update();
				m_JumpFloodPass[1]->Update();
				m_JumpFloodCompositePass->Update();
			});


			m_Renderer2D->Resize(m_Specification.Width, m_Specification.Height);
			m_NeedsResize = false;
		}

		m_Statistics = Statistics();

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

		SkyboxPass();
		GeometryPass();

		if (m_Options.JumpFlood)
			JumpFloodPass();


		m_CommandBuffer->BeginTimer("CompositePass");
		Renderer::BeginEventMarker(m_CommandBuffer, "Composite");
		
		Renderer::BeginRenderPass(m_CommandBuffer, m_CompositePass);
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_CompositePipeline, nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_CompositePass);

		Renderer::EndEventMarker(m_CommandBuffer);
		m_CommandBuffer->EndTimer("CompositePass");

		if (m_Options.JumpFlood)
		{
			Renderer::BeginEventMarker(m_CommandBuffer, "JumpFlood-Composite");

			Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodCompositePass);
			Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodCompositePipeline, nullptr);
			Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodCompositePass);

			Renderer::EndEventMarker(m_CommandBuffer);
		}

		m_CommandBuffer->End();
		m_CommandBuffer->Execute();

		m_Statistics.GPUTime = m_CommandBuffer->GetGPUExecutionTime();
		m_Statistics.GeometryPass = m_CommandBuffer->GetGPUExecutionTime("GeometryPass");
		m_Statistics.SkyboxPass = m_CommandBuffer->GetGPUExecutionTime("SkyboxPass");
		m_Statistics.CompositePass = m_CommandBuffer->GetGPUExecutionTime("CompositePass");
		m_Statistics.JumpFloodPass = m_CommandBuffer->GetGPUExecutionTime("JumpFloodPass");

		m_Scene = nullptr;
	}

	void SceneRenderer::SubmitMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<PBRMaterial> material, const glm::mat4& transform, int id)
	{
		SK_CORE_VERIFY(mesh);
		SK_CORE_VERIFY(material);

		// set concrete textures in the material
		material->PrepareAndUpdate();

		auto& meshData = m_DrawList.emplace_back();
		meshData.Mesh = mesh;
		meshData.MeshSource = meshSource;
		meshData.SubmeshIndex = submeshIndex;
		meshData.Material = material;
		meshData.Transform = transform;
		meshData.ID = id;
	}

	void SceneRenderer::SubmitSelectedMesh(Ref<Mesh> mesh, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<PBRMaterial> material, const glm::mat4& transform)
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
		SK_PROFILE_FUNCTION();

		// #idear #Renderer check if environment has changed
		Ref<Environment> environment = m_Scene->GetEnvironment();
		m_GeometryPass->SetInput("u_IrradianceMap", environment->GetIrradianceMap());
		m_GeometryPass->SetInput("u_RadianceMap", environment->GetRadianceMap());
		m_GeometryPass->Bake();

		m_SkyboxPass->SetInput("u_EnvironmentMap", environment->GetRadianceMap());
		m_SkyboxPass->Bake();

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
			m_SBPointLights->Resize(newCount);
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
		outlineSettings.Color = m_OutlineColor.xyz;
		outlineSettings.PixelWidth = m_OutlinePixelWidth;
		outlineSettings.TexelSize = { 1.0f / static_cast<float>(m_Specification.Width), 1.0f / static_cast<float>(m_Specification.Height) };
		Renderer::Submit([buffer = m_CBOutlineSettings, outlineSettings]()
		{
			buffer->RT_Upload(Buffer::FromValue(outlineSettings));
		});
	}

	void SceneRenderer::GeometryPass()
	{
		SK_PROFILE_FUNCTION();

		m_CommandBuffer->BeginTimer("GeometryPass");
		Renderer::BeginEventMarker(m_CommandBuffer, "Geometry Pass");

		Renderer::BeginRenderPass(m_CommandBuffer, m_GeometryPass);
		for (const auto& mesh : m_DrawList)
		{
			mesh.Material->PrepareAndUpdate();

			MeshPushConstant pcMesh;
			pcMesh.Transform = mesh.Transform;
			pcMesh.ID = mesh.ID;
			Renderer::RenderSubmeshWithMaterial(m_CommandBuffer, m_GeometryPipeline, mesh.Mesh, mesh.MeshSource, mesh.SubmeshIndex, mesh.Material->GetMaterial(), Buffer::FromValue(pcMesh));

			m_Statistics.DrawCalls++;
			m_Statistics.VertexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].VertexCount;
			m_Statistics.IndexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].IndexCount;
		}
		Renderer::EndRenderPass(m_CommandBuffer, m_GeometryPass);

		Renderer::BeginRenderPass(m_CommandBuffer, m_SelectedGeometryPass);
		for (const auto& mesh : m_SelectedDrawList)
		{
			Renderer::RenderSubmeshWithMaterial(m_CommandBuffer, m_SelectedGeometryPipeline, mesh.Mesh, mesh.MeshSource, mesh.SubmeshIndex, nullptr, Buffer::FromValue(mesh.Transform));

			m_Statistics.DrawCalls++;
			m_Statistics.VertexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].VertexCount;
			m_Statistics.IndexCount += mesh.MeshSource->GetSubmeshes()[mesh.SubmeshIndex].IndexCount;
		}
		Renderer::EndRenderPass(m_CommandBuffer, m_SelectedGeometryPass);


		Renderer::EndEventMarker(m_CommandBuffer);
		m_CommandBuffer->EndTimer("GeometryPass");
	}

	void SceneRenderer::SkyboxPass()
	{
		SK_PROFILE_FUNCTION();

		m_CommandBuffer->BeginTimer("SkyboxPass");
		Renderer::BeginRenderPass(m_CommandBuffer, m_SkyboxPass);
		Renderer::RenderCube(m_CommandBuffer, m_SkyboxPipeline, nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_SkyboxPass);
		m_CommandBuffer->EndTimer("SkyboxPass");
	}

	void SceneRenderer::JumpFloodPass()
	{
		SK_PROFILE_FUNCTION();

		m_CommandBuffer->BeginTimer("JumpFloodPass");
		Renderer::BeginEventMarker(m_CommandBuffer, "JumpFlood");

		Renderer::BeginRenderPass(m_CommandBuffer, m_JumpFloodInitPass);
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodInitPipeline, nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodInitPass);

		int steps = m_JumpFloodSteps;
		int step = glm::pow2(steps - 1);
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
			Renderer::RenderFullScreenQuad(m_CommandBuffer, m_JumpFloodPipeline, nullptr, vertexOverrides);
			Renderer::EndRenderPass(m_CommandBuffer, m_JumpFloodPass[index]);

			index = (index + 1) % 2;
			step /= 2;
		}

		vertexOverrides.Release();

		Renderer::EndEventMarker(m_CommandBuffer);
		m_CommandBuffer->EndTimer("JumpFloodPass");
	}

	void SceneRenderer::Initialize(const SceneRendererSpecification& specification)
	{
		SK_PROFILE_FUNCTION();

		m_Specification = specification;
		m_CommandBuffer = RenderCommandBuffer::Create(fmt::format("SceneRenderer - {}", specification.DebugName));

		m_CBScene             = ConstantBuffer::Create(sizeof(CBScene), "Scene");
		m_CBCamera            = ConstantBuffer::Create(sizeof(CBCamera), "Camera");
		m_CBSkybox            = ConstantBuffer::Create(sizeof(CBSkybox), "Skybox");
		m_CBSkyboxSettings    = ConstantBuffer::Create(sizeof(CBSkyboxSettings), "SkyboxSettings");
		m_CBCompositeSettings = ConstantBuffer::Create(sizeof(CBCompositeSettings), "CompositeSettings");
		m_CBOutlineSettings   = ConstantBuffer::Create(sizeof(CBOutlineSettings), "OutlineSettings");

		m_SBPointLights       = StorageBuffer::Create(sizeof(PointLight), 16, "Point Lights");
		m_SBDirectionalLights = StorageBuffer::Create(sizeof(DirectionalLight), LightEnvironment::MaxDirectionLights, "Directional Lights");


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
		mainFBSpecification.Attachments = { ImageFormat::RGBA32F, ImageFormat::RED32UI };
		mainFBSpecification.DepthAttachment = ImageFormat::Depth32;
		mainFBSpecification.ClearColor = m_ClearColor;
		mainFBSpecification.IndipendendClearColor[1] = std::numeric_limits<uint32_t>::max();

		if (specification.IsSwapchainTarget)
		{
			SK_NOT_IMPLEMENTED(); // #Renderer #Disabled #moro: not implemented
#if TODO
			Ref<FrameBuffer> swapchainFramebuffer = Application::Get().GetWindow().GetSwapChain()->GetFrameBuffer();
			mainFBSpecification.ExistingImages[0] = swapchainFramebuffer->GetImage(0);
#endif
		}

		Ref<FrameBuffer> clearFramebuffer = FrameBuffer::Create(mainFBSpecification);
		mainFBSpecification.ClearColorOnLoad = false;
		mainFBSpecification.ClearDepthOnLoad = false;
		mainFBSpecification.ExistingImages[0] = clearFramebuffer->GetImage(0);
		mainFBSpecification.ExistingImages[1] = clearFramebuffer->GetImage(1);
		mainFBSpecification.ExistingDepthImage = clearFramebuffer->GetDepthImage();
		Ref<FrameBuffer> loadFramebuffer = FrameBuffer::Create(mainFBSpecification);

		// Mesh
		{
			PipelineSpecification specification;
			specification.Shader = Renderer::GetShaderLibrary()->Get("SharkPBR");
			specification.Layout = layout;
			specification.DebugName = "PBR";
			specification.WireFrame = false;
			m_GeometryPipeline = Pipeline::Create(specification, loadFramebuffer->GetFramebufferInfo());

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Shader = specification.Shader;
			renderPassSpecification.TargetFramebuffer = loadFramebuffer;
			renderPassSpecification.DebugName = "PBR";
			m_GeometryPass = RenderPass::Create(renderPassSpecification);
			m_GeometryPass->SetInput("u_Camera", m_CBCamera);
			m_GeometryPass->SetInput("u_Scene", m_CBScene);
			m_GeometryPass->SetInput("u_PointLights", m_SBPointLights);
			m_GeometryPass->SetInput("u_DirectionalLights", m_SBDirectionalLights);
			m_GeometryPass->SetInput("u_IrradianceMap", Renderer::GetBlackTextureCube());
			m_GeometryPass->SetInput("u_RadianceMap", Renderer::GetBlackTextureCube());
			m_GeometryPass->SetInput("u_BRDFLUTTexture", Renderer::GetBRDFLUTTexture());
			m_GeometryPass->SetInput("u_EnvironmentSampler", Renderer::GetLinearClampSampler());
			SK_CORE_VERIFY(m_GeometryPass->Validate());
			m_GeometryPass->Bake();
		}

		// Skybox
		{
			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Skybox");
			pipelineSpecification.Layout = skyboxLayout;
			pipelineSpecification.DebugName = "Skybox";
			m_SkyboxPipeline = Pipeline::Create(pipelineSpecification, loadFramebuffer->GetFramebufferInfo());

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Shader = pipelineSpecification.Shader;
			renderPassSpecification.TargetFramebuffer = clearFramebuffer;
			renderPassSpecification.DebugName = pipelineSpecification.DebugName;
			m_SkyboxPass = RenderPass::Create(renderPassSpecification);
			m_SkyboxPass->SetInput("u_EnvironmentMap", Renderer::GetBlackTextureCube());
			m_SkyboxPass->SetInput("u_Uniforms", m_CBSkybox);
			m_SkyboxPass->SetInput("u_Settings", m_CBSkyboxSettings);
			SK_CORE_VERIFY(m_SkyboxPass->Validate());
			m_SkyboxPass->Bake();
		}

		// Selected Geometry
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.Width = specification.Width;
			framebufferSpecification.Height = specification.Height;
			framebufferSpecification.Attachments = { ImageFormat::RGBA32F };
			framebufferSpecification.DepthAttachment = ImageFormat::Depth32;
			framebufferSpecification.DebugName = "Selected Geometry";
			framebufferSpecification.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
			framebufferSpecification.ClearDepthValue = 1.0f;
			framebufferSpecification.ClearStencilValue = 0;
			auto framebuffer = FrameBuffer::Create(framebufferSpecification);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("SelectedGeometry");
			pipelineSpecification.Layout = layout;
			pipelineSpecification.DebugName = framebufferSpecification.DebugName;

			pipelineSpecification.EnableStencil = true;
			pipelineSpecification.StencilRef = 1;
			pipelineSpecification.StencilReadMask = 1;
			pipelineSpecification.StencilWriteMask = 1;
			pipelineSpecification.StencilComparisonOperator = CompareOperator::NotEqual;
			pipelineSpecification.StencilPassOperation = StencilOperation::Replace;
			m_SelectedGeometryPipeline = Pipeline::Create(pipelineSpecification, framebuffer->GetFramebufferInfo());

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Shader = pipelineSpecification.Shader;
			renderPassSpecification.TargetFramebuffer = framebuffer;
			renderPassSpecification.DebugName = pipelineSpecification.DebugName;

			m_SelectedGeometryPass = RenderPass::Create(renderPassSpecification);
			m_SelectedGeometryPass->SetInput("u_Camera", m_CBCamera);
			SK_CORE_VERIFY(m_SelectedGeometryPass->Validate());
			m_SelectedGeometryPass->Bake();
		}

		// Composite
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.Width = specification.Width;
			framebufferSpecification.Height = specification.Height;
			framebufferSpecification.Attachments = { ImageFormat::RGBA32F, ImageFormat::RED32SI };
			framebufferSpecification.DepthAttachment = ImageFormat::Depth32;

			framebufferSpecification.ExistingImages[1] = m_GeometryPass->GetOutput(1);
			framebufferSpecification.ExistingDepthImage = m_GeometryPass->GetDepthOutput();
			framebufferSpecification.ClearColorOnLoad = false;
			framebufferSpecification.ClearDepthOnLoad = false;
			framebufferSpecification.DebugName = "Composite";
			auto framebuffer = FrameBuffer::Create(framebufferSpecification);

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("Composite");
			pipelineSpecification.Layout = {
				{ VertexDataType::Float3, "Position" },
				{ VertexDataType::Float2, "TexCoord" },
			};
			pipelineSpecification.DebugName = framebufferSpecification.DebugName;
			pipelineSpecification.WriteDepth = false;
			m_CompositePipeline = Pipeline::Create(pipelineSpecification, framebuffer->GetFramebufferInfo());

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Shader = pipelineSpecification.Shader;
			renderPassSpecification.TargetFramebuffer = framebuffer;
			renderPassSpecification.DebugName = pipelineSpecification.DebugName;
			m_CompositePass = RenderPass::Create(renderPassSpecification);
			m_CompositePass->SetInput("u_Settings", m_CBCompositeSettings);
			m_CompositePass->SetInput("u_Input", m_SkyboxPass->GetOutput(0));
			SK_CORE_VERIFY(m_CompositePass->Validate());
			m_CompositePass->Bake();
		}

		// Temporary framebuffers
		{
			FrameBufferSpecification framebufferSpecification;
			framebufferSpecification.Width = m_Specification.Width;
			framebufferSpecification.Height = m_Specification.Height;
			framebufferSpecification.Attachments = { ImageFormat::RGBA32F };
			framebufferSpecification.ClearColor = { 0.5f, 0.1f, 0.1f, 1.0f };
			framebufferSpecification.DebugName = "Temporary";

			for (uint32_t i = 0; i < 2; i++)
				m_TempFramebuffers.push_back(FrameBuffer::Create(framebufferSpecification));
		}

		// Jump Flood Init
		{
			FrameBufferSpecification fbSpec;
			fbSpec.Width = m_Specification.Width;
			fbSpec.Height = m_Specification.Height;
			fbSpec.Attachments = { ImageFormat::RGBA32F };
			fbSpec.ExistingImages[0] = m_CompositePass->GetOutput(0);
			fbSpec.ClearColorOnLoad = false;
			fbSpec.DebugName = "JumpFlood-Composite";
			auto compositeFramebuffer = FrameBuffer::Create(fbSpec);

			auto initShader = Renderer::GetShaderLibrary()->Get("JumpFloodInit");
			auto passShader = Renderer::GetShaderLibrary()->Get("JumpFloodPass");
			auto compositeShader = Renderer::GetShaderLibrary()->Get("JumpFloodComposite");

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.BlendMode = FramebufferBlendMode::OneZero;
			pipelineSpecification.Layout = {
				{ VertexDataType::Float3, "Position" },
				{ VertexDataType::Float2, "TexCoord" }
			};

			pipelineSpecification.DebugName = "JumpFlood-Init";
			pipelineSpecification.Shader = initShader;
			m_JumpFloodInitPipeline = Pipeline::Create(pipelineSpecification, m_TempFramebuffers[0]->GetFramebufferInfo());

			pipelineSpecification.DebugName = "JumpFlood-Pass";
			pipelineSpecification.Shader = passShader;
			m_JumpFloodPipeline = Pipeline::Create(pipelineSpecification, m_TempFramebuffers[0]->GetFramebufferInfo());

			pipelineSpecification.DebugName = fbSpec.DebugName;
			pipelineSpecification.Shader = compositeShader;
			pipelineSpecification.DepthEnabled = false;
			pipelineSpecification.BlendMode = FramebufferBlendMode::SrcAlphaOneMinusSrcAlpha;
			m_JumpFloodCompositePipeline = Pipeline::Create(pipelineSpecification, compositeFramebuffer->GetFramebufferInfo());


			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Shader = initShader;
			renderPassSpecification.TargetFramebuffer = m_TempFramebuffers[0];
			renderPassSpecification.DebugName = "JumpFlood-Init";

			m_JumpFloodInitPass = RenderPass::Create(renderPassSpecification);
			m_JumpFloodInitPass->SetInput("u_Texture", m_SelectedGeometryPass->GetOutput(0));
			SK_CORE_VERIFY(m_JumpFloodInitPass->Validate());
			m_JumpFloodInitPass->Bake();

			const char* passNames[2] = { "EvenPass", "OddPass" };
			for (uint32_t i = 0; i < 2; i++)
			{
				renderPassSpecification.Shader = passShader;
				renderPassSpecification.TargetFramebuffer = m_TempFramebuffers[(i + 1) % 2];
				renderPassSpecification.DebugName = fmt::format("JumpFlood-{}", passNames[i]);

				m_JumpFloodPass[i] = RenderPass::Create(renderPassSpecification);
				m_JumpFloodPass[i]->SetInput("u_Texture", m_TempFramebuffers[i]->GetImage(0));
				SK_CORE_VERIFY(m_JumpFloodPass[i]->Validate());
				m_JumpFloodPass[i]->Bake();
			}

			renderPassSpecification.Shader = compositeShader;
			renderPassSpecification.TargetFramebuffer = compositeFramebuffer;
			renderPassSpecification.DebugName = fbSpec.DebugName;
			m_JumpFloodCompositePass = RenderPass::Create(renderPassSpecification);
			m_JumpFloodCompositePass->SetInput("u_Texture", m_TempFramebuffers[1]->GetImage(0));
			m_JumpFloodCompositePass->SetInput("u_Settings", m_CBOutlineSettings);
			SK_CORE_VERIFY(m_JumpFloodCompositePass->Validate());
			m_JumpFloodCompositePass->Bake();
		}

		if (specification.IsSwapchainTarget)
		{
			SK_NOT_IMPLEMENTED(); // #Renderer #moro not implemented
#if TODO
			Ref<SwapChain> swapchain = Application::Get().GetWindow().GetSwapChain();
			swapchain->AcknowledgeDependency(m_GeometryPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer);
			swapchain->AcknowledgeDependency(m_SkyboxPass->GetSpecification().Pipeline->GetSpecification().TargetFrameBuffer);
#endif
		}

		m_Renderer2D = Ref<Renderer2D>::Create(m_CompositePass->GetTargetFramebuffer());
	}

}
