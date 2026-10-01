
#include "Shark/Core/Application.h"
#include "Shark/Core/EntryPoint.h"

#include "Shark/Utils/PlatformUtils.h"
#include "Shark/Debug/Profiler.h"

#include "EditorLayer.h"

#include <choc/containers/choc_ArgumentList.h>

namespace Shark {

	std::filesystem::path GetStartupProject(choc::ArgumentList& args)
	{
		if (auto project = args.getExistingFileIfPresent("project", true))
			return std::move(*project);

		return std::filesystem::absolute(L"SandboxProject\\Sandbox.skproj");
	}

	void ValidateEnvironmentVariable()
	{
		const auto expected = std::filesystem::current_path().parent_path();
		auto var = Platform::GetEnvironmentVariable("SHARK_DIR");

		if (var == expected)
			return;

		if (var.empty())
		{
			SK_CORE_ERROR_TAG(Log::Tag::Core, "Environment variable 'SHARK_DIR' not set! Run Scripts/Setup again to fix this.");
		}
		else
		{
			SK_CORE_ERROR_TAG(Log::Tag::Core, "Environment variable 'SHARK_DIR' wrong! Run Scripts/Setup again to fix this.");
			SK_CORE_ERROR_TAG(Log::Tag::Core, "Got '{}' but expected '{}'", var, expected.generic_string());
		}

		std::terminate();
	}

	Application* CreateApplication(int argc, char** argv)
	{
		SK_PROFILE_FUNCTION();

		ValidateEnvironmentVariable();

		choc::ArgumentList args(argc, argv);

		ApplicationSpecification specification;
		specification.Name = "Shark-Editor";
		specification.WindowWidth = 1280;
		specification.WindowHeight = 720;
		specification.Maximized = true;
		specification.CustomTitlebar = true;
		specification.FullScreen = false;
		specification.EnableImGui = true;
		specification.VSync = true;

		auto application = sknew Application(specification, std::move(args));
		application->PushLayer(sknew EditorLayer(GetStartupProject(application->GetArgumentList())));
		return application;
	}

}