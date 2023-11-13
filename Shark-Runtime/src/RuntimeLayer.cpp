#include "skpch.h"
#include "RuntimeLayer.h"

#include "Shark/Core/Application.h"
#include "Shark/Core/Project.h"
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
		auto project = Project::LoadEditor(m_ProjectFile);
		if (!project)
		{
			Application::Get().CloseApplication();
			return;
		}

		Project::SetActive(project);
		ScriptEngine::LoadAssemblies(project->GetConfig().ScriptModulePath);
		
		m_Scene = AssetManager::GetAsset<Scene>(project->GetConfig().StartupScene);

		auto& window = Application::Get().GetWindow();
		m_Scene->SetViewportSize(window.GetWidth(), window.GetHeight());

		SceneRendererSpecification specification;
		specification.Width = window.GetWidth();
		specification.Height = window.GetHeight();
		specification.IsSwapchainTarget = true;
		specification.DebugName = "Viewport Renderer";
		m_Renderer = Ref<SceneRenderer>::Create(m_Scene, specification);
		m_Scene->OnScenePlay();
	}

	void RuntimeLayer::OnDetach()
	{
		m_Scene->OnSceneStop();

		m_Scene = nullptr;
		m_Renderer = nullptr;

		ScriptEngine::UnloadAssemblies();
		Project::SetActive(nullptr);
	}

	void RuntimeLayer::OnUpdate(TimeStep ts)
	{
		m_Scene->OnUpdateRuntime(ts);
		m_Scene->OnRenderRuntime(m_Renderer);
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

}
