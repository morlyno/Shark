#include <Shark.h>
#include <Shark/Core/EntryPoint.h>

#include "EditorLayer.h"
#include "Shark/Debug/Profiler.h"

namespace Shark {

	class EditorApplication : public Application
	{
	public:
		EditorApplication(const ApplicationSpecification& specification, std::string_view startupProject)
			: Application(specification), m_StartupProject(startupProject)
		{
			if (m_StartupProject.empty())
				m_StartupProject = std::filesystem::absolute(L"SandboxProject\\Sandbox.skproj");
		}

		virtual ~EditorApplication()
		{
		}

		virtual void OnInit() override
		{
			std::filesystem::path workingDirectory = std::filesystem::current_path();
			if (workingDirectory.stem() == L"Shark-Editor")
				workingDirectory = workingDirectory.parent_path();

			Platform::SetEnvironmentVariable("SHARK_DIR", workingDirectory.string());

			PushLayer(sknew EditorLayer(m_StartupProject));
		}

	private:
		std::filesystem::path m_StartupProject;
	};

	Application* CreateApplication(int argc, char** argv)
	{
		SK_PROFILE_FUNCTION();
		std::string_view startupProject;
		if (argc > 1)
			startupProject = argv[1];

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

		specification.ScriptConfig.CoreAssemblyPath = "Resources/Binaries/Shark-ScriptCore.dll";
		specification.ScriptConfig.EnableDebugging = true;
		specification.ScriptConfig.AutoReload = true;

		return sknew EditorApplication(specification, startupProject);
	}

}