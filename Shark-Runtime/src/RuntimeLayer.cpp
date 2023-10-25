#include "skpch.h"
#include "RuntimeLayer.h"

#include "Shark/Core/Project.h"
#include "Shark/Asset/ResourceManager.h"
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
		auto project = Ref<ProjectInstance>::Create();
		ProjectSerializer serializer(project);
		if (!serializer.Deserialize(m_ProjectFile))
		{
			Application::Get().CloseApplication();
			return;
		}

		Project::SetActive(project);
		ResourceManager::Init();
		ScriptEngine::LoadAssemblies(project->ScriptModulePath);
		
		AssetHandle startupScene = ResourceManager::GetAssetHandleFromFilePath(project->StartupScenePath);
		m_Scene = ResourceManager::GetAsset<Scene>(startupScene);

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
		ResourceManager::Shutdown();
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
