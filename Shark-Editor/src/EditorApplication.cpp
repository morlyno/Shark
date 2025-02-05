#include <Shark.h>
#include <Shark/Core/EntryPoint.h>

#include "EditorApplication.h"

#include "Shark/Debug/Profiler.h"

namespace Shark {

	Application* CreateApplication(int argc, char** argv)
	{
		SK_PROFILE_FUNCTION();
		std::string_view startupProject;
		if (argc > 1)
			startupProject = argv[1];
		else
			startupProject = "SandboxProject/Sandbox.skproj";

		RendererAPI::SetAPI(RendererAPIType::DirectX11);

		ApplicationSpecification specification;
		specification.Name = "Shark-Editor";
		specification.WindowWidth = 1280;
		specification.WindowHeight = 720;
		specification.Maximized = true;
		specification.CustomTitlebar = true;
		specification.FullScreen = false;
		specification.EnableImGui = true;
		specification.VSync = true;

		return sknew EditorApplication(specification, startupProject);
	}

	EditorApplication::EditorApplication(const ApplicationSpecification& specification, std::string_view startupProject)
		: Application(specification), m_StartupProject(startupProject)
	{
	}

	EditorApplication::~EditorApplication()
	{
	}

	void EditorApplication::OnInitialize()
	{
		std::filesystem::path workingDirectory = std::filesystem::current_path();
		if (workingDirectory.stem() == L"Shark-Editor")
			workingDirectory = workingDirectory.parent_path();

		Platform::SetEnvironmentVariable("SHARK_DIR", workingDirectory.string());

		m_ImGuiLayer = ImGuiLayer::Create();
		m_EditorLayer = sknew EditorLayer(m_StartupProject);

		PushLayer(m_ImGuiLayer);
		PushLayer(m_EditorLayer);
	}

	void EditorApplication::OnShutdown()
	{
		m_ImGuiLayer = nullptr;
		m_EditorLayer = nullptr;
	}

}