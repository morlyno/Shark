#include "skpch.h"

#include "Shark/Core/Application.h"
#include "Shark/Render/RendererAPI.h"
#include "Shark/File/FileSystem.h"

#include "Shark/Debug/Profiler.h"
#include "Shark/Core/EntryPoint.h"

#include "RuntimeLayer.h"

namespace Shark {

	class RuntimeApplication : public Application
	{
	public:
		RuntimeApplication(const ApplicationSpecification& specification, std::string_view startupProject)
			: Application(specification), m_StartupProject(startupProject)
		{
			if (m_StartupProject.empty())
				m_StartupProject = std::filesystem::absolute(L"SandboxProject\\Sandbox.skproj");
		}

		virtual void OnInit() override
		{
			PushLayer(new RuntimeLayer(m_StartupProject));
		}

	private:
		std::filesystem::path m_StartupProject;
	};

	Application* CreateApplication(int argc, char** argv)
	{
		std::string_view startupProject;
		if (argc > 1)
			startupProject = argv[1];

		std::filesystem::current_path("../Shark-Editor");

		RendererAPI::SetAPI(RendererAPIType::DirectX11);

		ApplicationSpecification specification;
		specification.Name = "Shark Runtime";
		specification.WindowWidth = 1280;
		specification.WindowHeight = 720;
		specification.Maximized = false;
		specification.Decorated = true;
		specification.FullScreen = false;
		specification.EnableImGui = false;
		specification.VSync = true;
		specification.IsRuntime = true;

#if 0
		specification.ScriptConfig.CoreAssemblyPath = "Resources/Binaries/Shark-ScriptCore.dll";
		specification.ScriptConfig.EnableDebugging = false;
		specification.ScriptConfig.AutoReload = false;
#endif

		return new RuntimeApplication(specification, startupProject);
	}

}
