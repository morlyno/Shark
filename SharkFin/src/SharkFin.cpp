#include "skfpch.h"

#include <Shark.h>
#include <Shark/Core/EntryPoint.h>

#include "EditorLayer.h"
#include "Shark/Debug/Instrumentor.h"

namespace Shark {

	class SharkFin : public Application
	{
	public:
		SharkFin(const ApplicationSpecification& specification, std::string_view startupProject)
			: Application(specification)
		{
			SK_PROFILE_FUNCTION();
		}

		virtual ~SharkFin()
		{
			SK_PROFILE_FUNCTION();
		}

		virtual void OnInit() override
		{
			SK_PROFILE_FUNCTION();

			if (m_StartupProject.empty())
				m_StartupProject = std::filesystem::absolute(L"SandboxProject\\SandboxProject.skproj");

			EditorLayer* editorlayer = new EditorLayer(m_StartupProject);
			PushLayer(editorlayer);
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

		RendererAPI::SetAPI(RendererAPI::API::DirectX11);

		ApplicationSpecification specification;
		specification.Name = "SharkFin";
		specification.WindowWidth = 1280;
		specification.WindowHeight = 720;
		specification.Maximized = true;
		specification.EnableImGui = true;
		specification.VSync = true;

		return new SharkFin(specification, startupProject);
	}

}