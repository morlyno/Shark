#include "skpch.h" 
#include "RuntimeLayer.h"

#include "Shark/Core/Application.h"
#include "Shark/Serialization/ProjectSerializer.h"
#include "Shark/Asset/AssetManager.h"
#include "Shark/Render/Renderer.h"

namespace Shark {

	RuntimeLayer::RuntimeLayer(const std::filesystem::path& projectFile)
		: Layer("Runtime Layer"), m_ProjectFile(projectFile)
	{
	}

	RuntimeLayer::~RuntimeLayer()
	{

	}

	void RuntimeLayer::OnAttach()
	{
		// TODO(moro): fix-me
		auto project = LoadProject(m_ProjectFile);
		if (!project)
		{
			Application::Get().CloseApplication();
			return;
		}

		Project::SetActive(project);
		ScriptEngine::Get().LoadAppAssembly();
		
		m_Scene = AssetManager::GetAsset<Scene>(project->StartupScene);
		ScriptEngine::Get().SetCurrentScene(m_Scene);

		auto& window = Application::Get().GetWindow();
		m_Scene->SetViewportSize(window.GetWidth(), window.GetHeight());

		SceneRendererSpecification specification;
		specification.Width = window.GetWidth();
		specification.Height = window.GetHeight();
		specification.IsSwapchainTarget = false;
		specification.DebugName = "Viewport Renderer";
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene, specification);
		m_Scene->OnScenePlay();

		m_CommandBuffer = RenderCommandBuffer::Create("Runtime");

		{
			auto swapchain = window.GetSwapChain();
			auto swapchainFB = swapchain->GetFrameBuffer();

			PipelineSpecification pipelineSpecification;
			pipelineSpecification.BackFaceCulling = true;
			pipelineSpecification.DepthEnabled = false;
			pipelineSpecification.TargetFrameBuffer = swapchainFB;
			pipelineSpecification.Shader = Renderer::GetShaderLibrary()->Get("BlitImage");
			pipelineSpecification.Layout = { { VertexDataType::Float3, "Position" }, { VertexDataType::Float2, "TexCoord" }, };
			pipelineSpecification.DebugName = "Present";

			RenderPassSpecification renderPassSpecification;
			renderPassSpecification.Pipeline = Pipeline::Create(pipelineSpecification);
			renderPassSpecification.DebugName = pipelineSpecification.DebugName;
			m_PresentPass = RenderPass::Create(renderPassSpecification);
			m_PresentPass->Set("u_SourceImage", m_Renderer->GetFinalPassImage());
			SK_CORE_VERIFY(m_PresentPass->Validate());
			m_PresentPass->Bake();
		}
	}

	void RuntimeLayer::OnDetach()
	{
		m_Scene->OnSceneStop();

		m_Scene = nullptr;
		m_Renderer = nullptr;

		m_CommandBuffer = nullptr;
		m_PresentPass = nullptr;

		Project::SetActive(nullptr);
	}

	void RuntimeLayer::OnUpdate(TimeStep ts)
	{
		m_Scene->OnUpdateRuntime(ts);
		m_Scene->OnRenderRuntime(m_Renderer);

		m_CommandBuffer->Begin();
		Renderer::BeginRenderPass(m_CommandBuffer, m_PresentPass);
		Renderer::RenderFullScreenQuad(m_CommandBuffer, m_PresentPass->GetPipeline(), nullptr);
		Renderer::EndRenderPass(m_CommandBuffer, m_PresentPass);
		m_CommandBuffer->End();
		m_CommandBuffer->Execute();
	}

	void RuntimeLayer::OnEvent(Event& event)
	{
		EventDispacher dispacher(event);
		dispacher.DispachEvent<WindowResizeEvent>(SK_BIND_EVENT_FN(RuntimeLayer::OnWindowResizedEvent));
		dispacher.DispachEvent<KeyPressedEvent>(SK_BIND_EVENT_FN(RuntimeLayer::OnKeyPressedEvent));
	}

	bool RuntimeLayer::OnWindowResizedEvent(WindowResizeEvent& event)
	{
		m_Scene->SetViewportSize(event.GetWidth(), event.GetHeight());
		m_Renderer->Resize(event.GetWidth(), event.GetHeight());

		return false;
	}

	bool RuntimeLayer::OnKeyPressedEvent(KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == KeyCode::F4 && event.GetModifierKeys().Alt)
		{
			Application::Get().CloseApplication();
			return true;
		}
		return false;
	}

	Ref<ProjectConfig> RuntimeLayer::LoadProject(const std::filesystem::path& projectFile)
	{
		auto project = Ref<ProjectConfig>::Create();
		ProjectSerializer serializer(project);
		serializer.Deserialize(projectFile);

		return project;
	}

}
